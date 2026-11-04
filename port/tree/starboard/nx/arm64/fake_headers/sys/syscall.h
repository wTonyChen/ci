// Nintendo Switch: newlib has no <sys/syscall.h> and Horizon has no syscall()
// entry point. perfetto's thread_utils.h uses it for SYS_gettid on the Linux
// code path (we define __linux__ so that path is taken), so provide the numbers
// and a tiny gettid-only dispatcher (see shims/posix_shim.cc).
#ifndef FAKE_SYS_SYSCALL_H_
#define FAKE_SYS_SYSCALL_H_

// asm-generic unistd.h numbers; the aarch64 table is the generic one.
#ifndef SYS_getpid
#define SYS_getpid 172
#endif
#ifndef SYS_gettid
#define SYS_gettid 178
#endif

#ifdef __cplusplus
extern "C" {
#endif

long syscall(long number, ...);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_SYSCALL_H_
