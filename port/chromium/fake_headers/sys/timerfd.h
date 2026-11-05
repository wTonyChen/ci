// Nintendo Switch: newlib has no <sys/timerfd.h> and Horizon has no timerfd.
// perfetto's periodic_task.cc needs the declarations to compile; the runtime
// side is a stub for now (see shims/posix_shim.cc) and must be replaced by a
// libnx timer + virtual-fd implementation (port/README.md, layer L3).
#ifndef FAKE_SYS_TIMERFD_H_
#define FAKE_SYS_TIMERFD_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TFD_NONBLOCK
#define TFD_NONBLOCK 00004000
#endif
#ifndef TFD_CLOEXEC
#define TFD_CLOEXEC 02000000
#endif
#ifndef TFD_TIMER_ABSTIME
#define TFD_TIMER_ABSTIME (1 << 0)
#endif
#ifndef TFD_TIMER_CANCEL_ON_SET
#define TFD_TIMER_CANCEL_ON_SET (1 << 1)
#endif

int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd,
                    int flags,
                    const struct itimerspec* new_value,
                    struct itimerspec* old_value);
int timerfd_gettime(int fd, struct itimerspec* curr_value);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_TIMERFD_H_
