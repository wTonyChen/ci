// Nintendo Switch: newlib has no sys/eventfd.h and Horizon has no eventfd.
// Compile-time surface only; the runtime implementation is a stub for now
// (see shims/posix_shim.cc and port/README.md, layer L3).
#ifndef FAKE_SYS_EVENTFD_H_
#define FAKE_SYS_EVENTFD_H_

#include <stdint.h>

typedef uint64_t eventfd_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EFD_SEMAPHORE
#define EFD_SEMAPHORE 1
#endif
#ifndef EFD_CLOEXEC
#define EFD_CLOEXEC 02000000
#endif
#ifndef EFD_NONBLOCK
#define EFD_NONBLOCK 00004000
#endif

int eventfd(unsigned int initval, int flags);
int eventfd_read(int fd, eventfd_t* value);
int eventfd_write(int fd, eventfd_t value);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_EVENTFD_H_
