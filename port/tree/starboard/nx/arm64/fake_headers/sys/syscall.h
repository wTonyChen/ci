// Nintendo Switch: newlib has no <sys/syscall.h> and Horizon has no syscall()
// entry point. perfetto's thread_utils.h uses it for SYS_gettid on the Linux
// code path (we define __linux__ so that path is taken), so provide the numbers
// and a tiny gettid-only dispatcher (see shims/posix_shim.cc).
#ifndef FAKE_SYS_SYSCALL_H_
#define FAKE_SYS_SYSCALL_H_

// asm-generic unistd.h numbers; the aarch64 table is the generic one.
// perfetto (thread_utils.h) uses __NR_gettid, partition_alloc uses
// __NR_getrandom, so both spellings are provided.
#ifndef SYS_getpid
#define SYS_getpid 172
#endif
#ifndef SYS_gettid
#define SYS_gettid 178
#endif
#ifndef SYS_getrandom
#define SYS_getrandom 278
#endif
#ifndef SYS_read
#define SYS_read 63
#endif
#ifndef SYS_write
#define SYS_write 64
#endif
#ifndef SYS_close
#define SYS_close 57
#endif
#ifndef SYS_exit
#define SYS_exit 93
#endif
#ifndef SYS_munmap
#define SYS_munmap 215
#endif
#ifndef SYS_mmap
#define SYS_mmap 222
#endif
#ifndef SYS_futex
#define SYS_futex 98
#endif

#ifndef __NR_getpid
#define __NR_getpid 172
#endif
#ifndef __NR_gettid
#define __NR_gettid 178
#endif
#ifndef __NR_getrandom
#define __NR_getrandom 278
#endif
#ifndef __NR_read
#define __NR_read 63
#endif
#ifndef __NR_write
#define __NR_write 64
#endif
#ifndef __NR_close
#define __NR_close 57
#endif
#ifndef __NR_exit
#define __NR_exit 93
#endif
#ifndef __NR_munmap
#define __NR_munmap 215
#endif
#ifndef __NR_mmap
#define __NR_mmap 222
#endif
#ifndef __NR_futex
#define __NR_futex 98
#endif

#ifdef __cplusplus
extern "C" {
#endif

long syscall(long number, ...);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_SYSCALL_H_
