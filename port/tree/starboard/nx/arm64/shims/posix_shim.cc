// Nintendo Switch POSIX gap fillers for the Chromium/Starboard target build.
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <switch.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>

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

}  // extern "C"
