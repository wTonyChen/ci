// Nintendo Switch POSIX gap fillers for the Chromium/Starboard target build.
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <dlfcn.h>
#include <switch.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/inotify.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <sched.h>
#include <stdarg.h>

extern "C" {

// perfetto's base/time.h calls timegm; newlib has no timegm.
time_t timegm(struct tm* tm) {
  return mktime(tm);
}

// Horizon has no eventfd. This is a compile/link placeholder only: once the
// virtual-fd layer (port/README.md, L1-L3) exists these must return a real
// counter fd, otherwise perfetto's event_fd.cc will fail at startup.
int eventfd(unsigned int initval, int flags) {
  (void)initval;
  (void)flags;
  errno = ENOSYS;
  return -1;
}

int eventfd_read(int fd, eventfd_t* value) {
  (void)fd;
  (void)value;
  errno = ENOSYS;
  return -1;
}

int eventfd_write(int fd, eventfd_t value) {
  (void)fd;
  (void)value;
  errno = ENOSYS;
  return -1;
}

// perfetto's thread_utils.h (Linux path) calls syscall(SYS_gettid). Horizon has
// no syscall instruction for this, but libnx gives us a stable thread handle.
long syscall(long number, ...) {
  switch (number) {
    case SYS_gettid:
      return static_cast<long>(threadGetCurHandle());
    default:
      errno = ENOSYS;
      return -1;
  }
}

// Horizon has no timerfd either; compile/link placeholder until the virtual-fd
// layer exists (port/README.md L3). perfetto's periodic_task.cc uses it.
int timerfd_create(int clockid, int flags) {
  (void)clockid;
  (void)flags;
  errno = ENOSYS;
  return -1;
}

int timerfd_settime(int fd,
                    int flags,
                    const struct itimerspec* new_value,
                    struct itimerspec* old_value) {
  (void)fd;
  (void)flags;
  (void)new_value;
  (void)old_value;
  errno = ENOSYS;
  return -1;
}

int timerfd_gettime(int fd, struct itimerspec* curr_value) {
  (void)fd;
  (void)curr_value;
  errno = ENOSYS;
  return -1;
}

// Horizon has no prctl(). Thread naming is cosmetic (no-op, report success);
// everything else -- notably the aarch64 MTE / tagged-address probes -- is
// advertised as unsupported so callers take their fallback path.
int prctl(int option, ...) {
  switch (option) {
    case PR_SET_NAME:
    case PR_SET_PDEATHSIG:
      // Nothing to do: thread names are cosmetic and we never fork, so a
      // parent-death signal has no meaning here.
      return 0;
    case PR_GET_NAME: {
      va_list ap;
      va_start(ap, option);
      char* name = va_arg(ap, char*);
      va_end(ap);
      if (name) {
        name[0] = '\0';
      }
      return 0;
    }
    default:
      errno = ENOSYS;
      return -1;
  }
}

// Horizon homebrew has no dynamic linker. Everything here fails cleanly so
// callers (base/third_party/symbolize) degrade to "no symbol names".
void* dlopen(const char* filename, int flags) {
  (void)filename;
  (void)flags;
  return NULL;
}

void* dlsym(void* handle, const char* symbol) {
  (void)handle;
  (void)symbol;
  return NULL;
}

int dlclose(void* handle) {
  (void)handle;
  return -1;
}

char* dlerror(void) {
  return const_cast<char*>("dynamic loading is not available on Switch");
}

int dladdr(const void* addr, void* info) {
  (void)addr;
  (void)info;
  return 0;
}

// glibc spelling of newlib's __errno(); Cobalt's modular ABI wrappers call it.
int* __errno_location(void) {
  return &errno;
}

// glibc-style getrandom() on top of libnx's CSRNG.
ssize_t getrandom(void* buf, size_t buflen, unsigned int flags) {
  (void)flags;
  if (buflen == 0) {
    return 0;
  }
  randomGet(buf, buflen);
  return static_cast<ssize_t>(buflen);
}

// Horizon has no inotify; report "not supported" rather than pretending.
int inotify_init(void) {
  errno = ENOSYS;
  return -1;
}

int inotify_init1(int flags) {
  (void)flags;
  errno = ENOSYS;
  return -1;
}

int inotify_add_watch(int fd, const char* pathname, uint32_t mask) {
  (void)fd;
  (void)pathname;
  (void)mask;
  errno = ENOSYS;
  return -1;
}

int inotify_rm_watch(int fd, int wd) {
  (void)fd;
  (void)wd;
  errno = ENOSYS;
  return -1;
}

// newlib's pthread has no scopes / scheduling policy / getattr_np. These are
// called by Cobalt's modular ABI wrappers; report "not supported" honestly.
int pthread_getattr_np(pthread_t thread, pthread_attr_t* attr) {
  (void)thread;
  (void)attr;
  return ENOSYS;
}

int pthread_attr_getscope(const pthread_attr_t* attr, int* scope) {
  (void)attr;
  if (scope) {
    *scope = 0;
  }
  return ENOSYS;
}

int pthread_attr_setscope(pthread_attr_t* attr, int scope) {
  (void)attr;
  (void)scope;
  return ENOSYS;
}

int pthread_attr_getschedpolicy(const pthread_attr_t* attr, int* policy) {
  (void)attr;
  if (policy) {
    *policy = 0;
  }
  return ENOSYS;
}

int pthread_attr_setschedpolicy(pthread_attr_t* attr, int policy) {
  (void)attr;
  (void)policy;
  return ENOSYS;
}

int pthread_getschedparam(pthread_t thread, int* policy,
                          struct sched_param* param) {
  (void)thread;
  (void)param;
  if (policy) {
    *policy = 0;
  }
  return ENOSYS;
}

int pthread_setschedparam(pthread_t thread, int policy,
                          const struct sched_param* param) {
  (void)thread;
  (void)policy;
  (void)param;
  return ENOSYS;
}

// Horizon exposes no rlimits or process priorities; report "not supported".
int getrlimit(int resource, struct rlimit* rlim) {
  (void)resource;
  (void)rlim;
  errno = ENOSYS;
  return -1;
}

int setrlimit(int resource, const struct rlimit* rlim) {
  (void)resource;
  (void)rlim;
  errno = ENOSYS;
  return -1;
}

int getpriority(int which, int who) {
  (void)which;
  (void)who;
  errno = ENOSYS;
  return -1;
}

int setpriority(int which, int who, int prio) {
  (void)which;
  (void)who;
  (void)prio;
  errno = ENOSYS;
  return -1;
}

// Switch has one scheduler policy and no CPU affinity control; be explicit.
int sched_setscheduler(pid_t pid, int policy, const struct sched_param* param) {
  (void)pid;
  (void)policy;
  (void)param;
  errno = ENOSYS;
  return -1;
}

int sched_getscheduler(pid_t pid) {
  (void)pid;
  errno = ENOSYS;
  return -1;
}

int sched_setparam(pid_t pid, const struct sched_param* param) {
  (void)pid;
  (void)param;
  errno = ENOSYS;
  return -1;
}

int sched_getparam(pid_t pid, struct sched_param* param) {
  (void)pid;
  (void)param;
  errno = ENOSYS;
  return -1;
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t* mask) {
  (void)pid;
  (void)cpusetsize;
  (void)mask;
  errno = ENOSYS;
  return -1;
}

int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t* mask) {
  (void)pid;
  (void)cpusetsize;
  (void)mask;
  errno = ENOSYS;
  return -1;
}

// Horizon has no getifaddrs(); Cobalt's socket wrapper degrades gracefully.
int getifaddrs(struct ifaddrs** ifap) {
  if (ifap) {
    *ifap = nullptr;
  }
  errno = ENOSYS;
  return -1;
}

void freeifaddrs(struct ifaddrs* ifa) {
  (void)ifa;
}

// No interface index space on Switch; POSIX says 0 means failure.
unsigned int if_nametoindex(const char* ifname) {
  (void)ifname;
  return 0;
}

// No interface index space on Switch; POSIX returns NULL on failure.
char* if_indextoname(unsigned int ifindex, char* ifname) {
  (void)ifindex;
  (void)ifname;
  return nullptr;
}

}  // extern "C"
