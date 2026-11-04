// Nintendo Switch: newlib has no <sys/inotify.h> and Horizon has no inotify.
// Cobalt's modular inotify ABI wrapper needs the declarations/constants to
// compile; the shim answers ENOSYS (no filesystem watch support on Switch).
#ifndef FAKE_SYS_INOTIFY_H_
#define FAKE_SYS_INOTIFY_H_

#include <stdint.h>

#ifndef IN_CLOEXEC
#define IN_CLOEXEC 02000000
#endif
#ifndef IN_NONBLOCK
#define IN_NONBLOCK 00004000
#endif

#ifndef IN_ACCESS
#define IN_ACCESS 0x00000001
#endif
#ifndef IN_MODIFY
#define IN_MODIFY 0x00000002
#endif
#ifndef IN_ATTRIB
#define IN_ATTRIB 0x00000004
#endif
#ifndef IN_CLOSE_WRITE
#define IN_CLOSE_WRITE 0x00000008
#endif
#ifndef IN_CLOSE_NOWRITE
#define IN_CLOSE_NOWRITE 0x00000010
#endif
#ifndef IN_OPEN
#define IN_OPEN 0x00000020
#endif
#ifndef IN_MOVED_FROM
#define IN_MOVED_FROM 0x00000040
#endif
#ifndef IN_MOVED_TO
#define IN_MOVED_TO 0x00000080
#endif
#ifndef IN_CREATE
#define IN_CREATE 0x00000100
#endif
#ifndef IN_DELETE
#define IN_DELETE 0x00000200
#endif
#ifndef IN_DELETE_SELF
#define IN_DELETE_SELF 0x00000400
#endif
#ifndef IN_MOVE_SELF
#define IN_MOVE_SELF 0x00000800
#endif
#ifndef IN_UNMOUNT
#define IN_UNMOUNT 0x00002000
#endif
#ifndef IN_Q_OVERFLOW
#define IN_Q_OVERFLOW 0x00004000
#endif
#ifndef IN_MASK_ADD
#define IN_MASK_ADD 0x20000000
#endif
#ifndef IN_EXCL_UNLINK
#define IN_EXCL_UNLINK 0x04000000
#endif
#ifndef IN_ALL_EVENTS
#define IN_ALL_EVENTS 0x00000FFF
#endif
#ifndef IN_IGNORED
#define IN_IGNORED 0x00008000
#endif
#ifndef IN_ISDIR
#define IN_ISDIR 0x40000000
#endif
#ifndef IN_ONESHOT
#define IN_ONESHOT 0x80000000
#endif
#ifndef IN_ONLYDIR
#define IN_ONLYDIR 0x01000000
#endif
#ifndef IN_DONT_FOLLOW
#define IN_DONT_FOLLOW 0x02000000
#endif

#ifdef __cplusplus
extern "C" {
#endif

int inotify_init(void);
int inotify_init1(int flags);
int inotify_add_watch(int fd, const char* pathname, uint32_t mask);
int inotify_rm_watch(int fd, int wd);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_INOTIFY_H_
