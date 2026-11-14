// L1-L3 of the libnx POSIX layer (port/README.md): the virtual fd table that
// gives Chromium's message loop the "an fd you can poll, write to wake, read to
// drain" primitive it assumes exists.
//
// Why this is not optional: Horizon has no pipe/eventfd/timerfd. libnx only
// answers poll() for bsd:u sockets, and base::MessagePumpDefault is built
// entirely around an eventfd + poll(). Without this file, base::MessageLoop
// cannot even be constructed on the Switch.
//
// Design (kept deliberately small, one object kind at a time):
//   * virtual fds live at 0x40000000+n, so they never collide with libnx fds;
//   * poll()/read()/write()/close()/fcntl() are wrapped with --wrap=<name>:
//     virtual fds are handled here, everything else goes to __real_<name>,
//     which keeps sockets and files working unchanged;
//   * eventfd: 64-bit counter, read drains (or EFD_SEMAPHORE: takes 1),
//     write adds, POLLIN while the counter is non-zero;
//   * timerfd: one-shot/repeating deadline, POLLIN once expired, read consumes
//     and returns the number of expirations;
//   * pipe2: 16 KiB ring buffer shared by the read end and the write end.
//
// Not here yet (next stages, each with its own probe): epoll, socketpair,
// EPOLLONESHOT/EPOLLET emulation.
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <switch.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

constexpr int kFdBase = 0x40000000;
constexpr int kMaxObjs = 24;
constexpr int kPipeCount = 4;
constexpr size_t kPipeCap = 16 * 1024;

enum Kind : unsigned char {
  kFree = 0,
  kEventFd,
  kTimerFd,
  kPipeRead,
  kPipeWrite,
};

struct Obj {
  Kind kind = kFree;
  int flags = 0;              // O_NONBLOCK et al.
  uint64_t counter = 0;       // eventfd
  uint64_t interval_ns = 0;   // timerfd
  uint64_t next_ns = 0;       // timerfd deadline
  bool armed = false;
  int peer = -1;              // pipe: the other end
  int buf_index = -1;         // pipe: slot in g_pipe_bufs
  size_t head = 0;            // pipe: read offset
  size_t len = 0;             // pipe: bytes available
};

Obj g_objs[kMaxObjs];
unsigned char g_pipe_bufs[kPipeCount][kPipeCap];
bool g_pipe_buf_used[kPipeCount];

int IndexOf(int fd) {
  if (fd < kFdBase || fd >= kFdBase + kMaxObjs) {
    return -1;
  }
  const int i = fd - kFdBase;
  return g_objs[i].kind == kFree ? -1 : i;
}

int Alloc(Kind kind) {
  for (int i = 0; i < kMaxObjs; ++i) {
    if (g_objs[i].kind == kFree) {
      g_objs[i] = Obj();
      g_objs[i].kind = kind;
      return kFdBase + i;
    }
  }
  errno = EMFILE;
  return -1;
}

void Free(int i) {
  if (g_objs[i].kind == kPipeRead || g_objs[i].kind == kPipeWrite) {
    const int b = g_objs[i].buf_index;
    const int peer = g_objs[i].peer;
    if (b >= 0) {
      g_pipe_buf_used[b] = false;
    }
    if (peer != -1) {
      const int j = IndexOf(peer);
      if (j >= 0) {
        g_objs[j].peer = -1;  // the surviving end sees EOF/EPIPE
      }
    }
  }
  g_objs[i] = Obj();
}

uint64_t NowNs() {
  return armGetSystemTick() * 625 / 12;  // 19.2 MHz ticks -> nanoseconds
}

// 0 = not ready, 1 = ready. `events` is a poll event mask.
int Readiness(int i, short events) {
  const Obj& o = g_objs[i];
  switch (o.kind) {
    case kEventFd: {
      short r = 0;
      if (o.counter != 0) r |= POLLIN;
      r |= POLLOUT;  // writes never block (counter saturates)
      return (events & r) ? 1 : 0;
    }
    case kTimerFd:
      return (o.armed && NowNs() >= o.next_ns && (events & POLLIN)) ? 1 : 0;
    case kPipeRead: {
      if (o.len > 0) return (events & POLLIN) ? 1 : 0;
      if (o.peer == -1) return (events & POLLIN) ? 1 : 0;  // EOF is readable
      return 0;
    }
    case kPipeWrite: {
      if (o.peer == -1) return (events & POLLOUT) ? 1 : 0;  // EPIPE, report it
      const Obj& r = g_objs[IndexOf(o.peer)];
      return (r.len < kPipeCap && (events & POLLOUT)) ? 1 : 0;
    }
    default:
      return 0;
  }
}

short PollRevents(int i, short events) {
  const Obj& o = g_objs[i];
  switch (o.kind) {
    case kEventFd:
      return (o.counter ? POLLIN : 0) | POLLOUT;
    case kTimerFd:
      return (o.armed && NowNs() >= o.next_ns) ? POLLIN : 0;
    case kPipeRead:
      return (o.len > 0 || o.peer == -1) ? POLLIN : 0;
    case kPipeWrite:
      return (o.peer == -1) ? POLLOUT : 0;
    default:
      (void)events;
      return 0;
  }
}

}  // namespace

extern "C" {

int __real_poll(struct pollfd* fds, nfds_t nfds, int timeout);
ssize_t __real_read(int fd, void* buf, size_t count);
ssize_t __real_write(int fd, const void* buf, size_t count);
int __real_close(int fd);
int __real_fcntl(int fd, int cmd, ...);
ssize_t __wrap_read(int fd, void* buf, size_t count);
ssize_t __wrap_write(int fd, const void* buf, size_t count);

// ---- L3 objects -----------------------------------------------------------

int eventfd(unsigned int initval, int flags) {
  const int fd = Alloc(kEventFd);
  if (fd < 0) return -1;
  Obj& o = g_objs[fd - kFdBase];
  o.counter = initval;
  o.flags = (flags & EFD_NONBLOCK) ? O_NONBLOCK : 0;
  return fd;
}

// glibc's helpers; Chromium's base/posix/eintr_wrapper + event_fd use these.
int eventfd_read(int fd, eventfd_t* value) {
  return __wrap_read(fd, value, sizeof(*value)) == 8 ? 0 : -1;
}

int eventfd_write(int fd, eventfd_t value) {
  return __wrap_write(fd, &value, sizeof(value)) == 8 ? 0 : -1;
}

int timerfd_create(int clockid, int flags) {
  (void)clockid;  // CLOCK_MONOTONIC is all anyone uses here
  const int fd = Alloc(kTimerFd);
  if (fd < 0) return -1;
  g_objs[fd - kFdBase].flags = (flags & TFD_NONBLOCK) ? O_NONBLOCK : 0;
  return fd;
}

int timerfd_settime(int fd, int flags, const struct itimerspec* new_value,
                    struct itimerspec* old_value) {
  const int i = IndexOf(fd);
  if (i < 0 || g_objs[i].kind != kTimerFd) {
    errno = EINVAL;
    return -1;
  }
  Obj& o = g_objs[i];
  if (old_value) {
    old_value->it_value.tv_sec = 0;
    old_value->it_value.tv_nsec = 0;
    old_value->it_interval.tv_sec =
        static_cast<time_t>(o.interval_ns / 1000000000ULL);
    old_value->it_interval.tv_nsec =
        static_cast<long>(o.interval_ns % 1000000000ULL);
  }
  const uint64_t rel =
      static_cast<uint64_t>(new_value->it_value.tv_sec) * 1000000000ULL +
      static_cast<uint64_t>(new_value->it_value.tv_nsec);
  o.interval_ns =
      static_cast<uint64_t>(new_value->it_interval.tv_sec) * 1000000000ULL +
      static_cast<uint64_t>(new_value->it_interval.tv_nsec);
  o.armed = rel != 0;
  o.next_ns = NowNs() + (flags & TFD_TIMER_ABSTIME ? 0 : rel);
  if (flags & TFD_TIMER_ABSTIME) {
    o.next_ns = rel;
  }
  return 0;
}

int timerfd_gettime(int fd, struct itimerspec* curr_value) {
  const int i = IndexOf(fd);
  if (i < 0 || g_objs[i].kind != kTimerFd) {
    errno = EINVAL;
    return -1;
  }
  const Obj& o = g_objs[i];
  const uint64_t left = o.armed && o.next_ns > NowNs() ? o.next_ns - NowNs() : 0;
  curr_value->it_value.tv_sec = static_cast<time_t>(left / 1000000000ULL);
  curr_value->it_value.tv_nsec = static_cast<long>(left % 1000000000ULL);
  curr_value->it_interval.tv_sec =
      static_cast<time_t>(o.interval_ns / 1000000000ULL);
  curr_value->it_interval.tv_nsec =
      static_cast<long>(o.interval_ns % 1000000000ULL);
  return 0;
}

int pipe2(int fds[2], int flags) {
  int buf = -1;
  for (int b = 0; b < kPipeCount; ++b) {
    if (!g_pipe_buf_used[b]) {
      buf = b;
      break;
    }
  }
  if (buf < 0) {
    errno = ENFILE;
    return -1;
  }
  const int r = Alloc(kPipeRead);
  const int w = Alloc(kPipeWrite);
  if (r < 0 || w < 0) {
    if (r >= 0) Free(r - kFdBase);
    if (w >= 0) Free(w - kFdBase);
    errno = EMFILE;
    return -1;
  }
  g_pipe_buf_used[buf] = true;
  Obj& ro = g_objs[r - kFdBase];
  Obj& wo = g_objs[w - kFdBase];
  ro.peer = w;
  wo.peer = r;
  ro.buf_index = buf;
  wo.buf_index = buf;
  const int fl = (flags & O_NONBLOCK) ? O_NONBLOCK : 0;
  ro.flags = fl;
  wo.flags = fl;
  fds[0] = r;
  fds[1] = w;
  return 0;
}

// ---- L1 wrappers ----------------------------------------------------------

ssize_t __wrap_read(int fd, void* buf, size_t count) {
  const int i = IndexOf(fd);
  if (i < 0) return __real_read(fd, buf, count);
  Obj& o = g_objs[i];
  switch (o.kind) {
    case kEventFd: {
      if (count < 8) {
        errno = EINVAL;
        return -1;
      }
      if (o.counter == 0) {
        if (o.flags & O_NONBLOCK) {
          errno = EAGAIN;
          return -1;
        }
        while (o.counter == 0) svcSleepThread(2000000ULL);  // 2 ms
      }
      uint64_t v = o.counter;
      o.counter = 0;
      memcpy(buf, &v, 8);
      return 8;
    }
    case kTimerFd: {
      if (count < 8) {
        errno = EINVAL;
        return -1;
      }
      if (!o.armed || NowNs() < o.next_ns) {
        if (o.flags & O_NONBLOCK) {
          errno = EAGAIN;
          return -1;
        }
        while (!o.armed || NowNs() < o.next_ns) svcSleepThread(2000000ULL);
      }
      uint64_t v = 1;
      if (o.interval_ns) {
        o.next_ns += o.interval_ns;
      } else {
        o.armed = false;
      }
      memcpy(buf, &v, 8);
      return 8;
    }
    case kPipeRead: {
      if (o.len == 0) {
        if (o.peer == -1) return 0;  // EOF
        if (o.flags & O_NONBLOCK) {
          errno = EAGAIN;
          return -1;
        }
        while (o.len == 0 && o.peer != -1) svcSleepThread(2000000ULL);
        if (o.len == 0) return 0;
      }
      unsigned char* base = g_pipe_bufs[o.buf_index];
      size_t n = count < o.len ? count : o.len;
      for (size_t k = 0; k < n; ++k) {
        reinterpret_cast<unsigned char*>(buf)[k] = base[(o.head + k) % kPipeCap];
      }
      o.head = (o.head + n) % kPipeCap;
      o.len -= n;
      return static_cast<ssize_t>(n);
    }
    default:
      errno = EINVAL;
      return -1;
  }
}

ssize_t __wrap_write(int fd, const void* buf, size_t count) {
  const int i = IndexOf(fd);
  if (i < 0) return __real_write(fd, buf, count);
  Obj& o = g_objs[i];
  switch (o.kind) {
    case kEventFd: {
      if (count < 8) {
        errno = EINVAL;
        return -1;
      }
      uint64_t v = 0;
      memcpy(&v, buf, 8);
      o.counter += v;  // saturating in the kernel; close enough here
      return 8;
    }
    case kPipeWrite: {
      if (o.peer == -1) {
        errno = EPIPE;
        return -1;
      }
      Obj& r = g_objs[IndexOf(o.peer)];
      unsigned char* base = g_pipe_bufs[r.buf_index];
      size_t n = 0;
      while (n < count && r.len < kPipeCap) {
        base[(r.head + r.len) % kPipeCap] =
            reinterpret_cast<const unsigned char*>(buf)[n];
        ++r.len;
        ++n;
      }
      if (n == 0) {
        if (o.flags & O_NONBLOCK) {
          errno = EAGAIN;
          return -1;
        }
        while (r.len < kPipeCap) svcSleepThread(2000000ULL);
        return __wrap_write(fd, buf, count);
      }
      return static_cast<ssize_t>(n);
    }
    default:
      errno = EINVAL;
      return -1;
  }
}

int __wrap_close(int fd) {
  const int i = IndexOf(fd);
  if (i < 0) return __real_close(fd);
  Free(i);
  return 0;
}

int __wrap_fcntl(int fd, int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  const long arg = va_arg(ap, long);
  va_end(ap);
  const int i = IndexOf(fd);
  if (i < 0) return __real_fcntl(fd, cmd, arg);
  Obj& o = g_objs[i];
  switch (cmd) {
    case F_GETFL:
      return o.flags;
    case F_SETFL:
      o.flags = static_cast<int>(arg);
      return 0;
    case F_GETFD:
      return 0;
    case F_SETFD:
      return 0;
    default:
      errno = EINVAL;
      return -1;
  }
}

// Merges real fds (straight to libnx) with virtual fds (Readiness above).
// Level-triggered, like POSIX: callers re-poll after handling.
int __wrap_poll(struct pollfd* fds, nfds_t nfds, int timeout) {
  struct pollfd real[32];
  int real_idx[32];
  int nreal = 0;
  for (nfds_t k = 0; k < nfds; ++k) {
    fds[k].revents = 0;
    if (IndexOf(fds[k].fd) < 0) {
      if (nreal < 32) {
        real[nreal] = fds[k];
        real_idx[nreal] = static_cast<int>(k);
        ++nreal;
      }
    }
  }

  const uint64_t deadline =
      timeout < 0 ? 0 : NowNs() + static_cast<uint64_t>(timeout) * 1000000ULL;

  for (;;) {
    int ready = 0;
    for (nfds_t k = 0; k < nfds; ++k) {
      const int i = IndexOf(fds[k].fd);
      if (i >= 0 && Readiness(i, fds[k].events)) {
        fds[k].revents = PollRevents(i, fds[k].events);
        ++ready;
      }
    }

    int remaining = 0;
    bool expired = false;
    if (timeout >= 0) {
      const uint64_t now = NowNs();
      expired = now >= deadline;
      const uint64_t left_ns = expired ? 0 : deadline - now;
      remaining = static_cast<int>((left_ns + 999999ULL) / 1000000ULL);
    }

    if (ready > 0 || nreal > 0 || expired) {
      if (nreal > 0) {
        const int n = __real_poll(real, nreal, ready > 0 ? 0 : remaining);
        if (n > 0) {
          for (int k = 0; k < nreal; ++k) {
            if (real[k].revents) {
              fds[real_idx[k]].revents = real[k].revents;
              ++ready;
            }
          }
        }
      }
    }

    if (ready > 0) return ready;
    if (timeout == 0 || expired) return 0;
    svcSleepThread(1000000ULL);  // 1 ms: no blocking primitive for a mixed set
  }
}

}  // extern "C"
