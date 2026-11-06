// Nintendo Switch: newlib has no <sys/syscall.h> and Horizon has no syscall()
// entry point. Chromium's Linux code paths reference the aarch64 syscall
// numbers (mostly to call getrandom/gettid directly); provide a useful subset
// of the asm-generic table, in both spellings.
#ifndef FAKE_SYS_SYSCALL_H_
#define FAKE_SYS_SYSCALL_H_

#define SYS_ioctl 29
#define SYS_openat 56
#define SYS_close 57
#define SYS_getdents64 61
#define SYS_lseek 62
#define SYS_read 63
#define SYS_write 64
#define SYS_newfstatat 79
#define SYS_fstat 80
#define SYS_exit 93
#define SYS_exit_group 94
#define SYS_futex 98
#define SYS_sched_getaffinity 123
#define SYS_rt_sigprocmask 135
#define SYS_tgkill 131
#define SYS_getpid 172
#define SYS_gettid 178
#define SYS_munmap 215
#define SYS_clone 220
#define SYS_mmap 222
#define SYS_prlimit64 261
#define SYS_getrandom 278
#define SYS_memfd_create 279
#define SYS_statx 291
#define SYS_rseq 293
#define SYS_clone3 435

#define __NR_ioctl 29
#define __NR_openat 56
#define __NR_close 57
#define __NR_getdents64 61
#define __NR_lseek 62
#define __NR_read 63
#define __NR_write 64
#define __NR_newfstatat 79
#define __NR_fstat 80
#define __NR_exit 93
#define __NR_exit_group 94
#define __NR_futex 98
#define __NR_sched_getaffinity 123
#define __NR_rt_sigprocmask 135
#define __NR_tgkill 131
#define __NR_getpid 172
#define __NR_gettid 178
#define __NR_munmap 215
#define __NR_clone 220
#define __NR_mmap 222
#define __NR_prlimit64 261
#define __NR_getrandom 278
#define __NR_memfd_create 279
#define __NR_statx 291
#define __NR_rseq 293
#define __NR_clone3 435

#ifdef __cplusplus
extern "C" {
#endif
long syscall(long number, ...);
#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_SYSCALL_H_
