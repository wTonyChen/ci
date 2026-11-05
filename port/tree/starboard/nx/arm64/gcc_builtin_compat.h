#ifndef GCC_BUILTIN_COMPAT_H_
#define GCC_BUILTIN_COMPAT_H_
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 14)
#define __builtin_clzg(x, y) \
  _Generic((x), \
    unsigned int: __builtin_clz(x), \
    unsigned long: __builtin_clzl(x), \
    unsigned long long: __builtin_clzll(x), \
    default: __builtin_clz((unsigned int)(x)) \
  )
#define __builtin_popcountg(x) \
  _Generic((x), \
    unsigned int: __builtin_popcount(x), \
    unsigned long: __builtin_popcountl(x), \
    unsigned long long: __builtin_popcountll(x), \
    default: __builtin_popcount((unsigned int)(x)) \
  )
#endif
#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID 3
#endif
#ifndef SA_NOCLDWAIT
#define SA_NOCLDWAIT 0x20000000
#endif
#ifndef SA_ONSTACK
#define SA_ONSTACK 0x08000000
#endif
#ifndef SA_SIGINFO
#define SA_SIGINFO 4
#endif
#ifndef SA_NODEFER
#define SA_NODEFER 0x40000000
#endif
#ifndef SA_RESTART
#define SA_RESTART 0x10000000
#endif
#ifndef SA_RESETHAND
#define SA_RESETHAND 0x80000000
#endif
// newlib has no timegm(); the definition lives in shims/posix_shim.cc. This
// header is force-included into every target C++ compile (GN_CXX_FLAGS
// -include), so the declaration is visible where perfetto needs it.
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
time_t timegm(struct tm* tm);
#ifdef __cplusplus
}
#endif

// Cobalt's modular ABI wrapper calls glibc's __errno_location(); newlib exposes
// the same thing as __errno(). Declared here (force-included) and defined in
// shims/posix_shim.cc.
#ifdef __cplusplus
extern "C" {
#endif
int* __errno_location(void);
#ifdef __cplusplus
}
#endif

// partition_alloc's stack_trace_posix.cc gets ElfW()/ELFMAG/SELFMAG from
// glibc's <link.h>/<elf.h>. newlib has <elf.h> but neither ElfW nor SELFMAG,
// and no <link.h> at all. Fill the gaps here (aarch64 is LP64 -> Elf64_*).
#include <elf.h>
#ifndef SELFMAG
#define SELFMAG 4
#endif
#ifndef ElfW
#define ElfW(type) Elf64_##type
#endif
// Cobalt's modular fcntl wrappers reference Linux-only open/fcntl constants
// that newlib does not define. Values are musl's (aarch64).
#ifndef O_PATH
#define O_PATH 010000000
#endif
#ifndef O_TMPFILE
#define O_TMPFILE 020200000
#endif
#ifndef F_GETOWN_EX
#define F_GETOWN_EX 0x7E01  /* newlib reuses 15/16 for F_OFD_* */
#endif
#ifndef F_SETOWN_EX
#define F_SETOWN_EX 0x7E02
#endif
// newlib's pthread is a stub: no scopes / scheduling policy / getattr_np.
// Cobalt's modular pthread ABI wrappers call these glibc/POSIX extensions, so
// declare them here and let shims/posix_shim.cc answer ENOSYS.
#include <pthread.h>
#include <sched.h>
#ifdef __cplusplus
extern "C" {
#endif
int pthread_getattr_np(pthread_t thread, pthread_attr_t* attr);
int pthread_attr_getscope(const pthread_attr_t* attr, int* scope);
int pthread_attr_setscope(pthread_attr_t* attr, int scope);
int pthread_attr_getschedpolicy(const pthread_attr_t* attr, int* policy);
int pthread_attr_setschedpolicy(pthread_attr_t* attr, int policy);
int pthread_getschedparam(pthread_t thread, int* policy,
                          struct sched_param* param);
int pthread_setschedparam(pthread_t thread, int policy,
                          const struct sched_param* param);
#ifdef __cplusplus
}
#endif
#ifndef PTHREAD_PROCESS_PRIVATE
#define PTHREAD_PROCESS_PRIVATE 0
#endif
#ifndef PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED 1
#endif
// newlib's <sys/resource.h> only has rusage/getrusage: no rlimit, no
// PRIO_*/getpriority/setpriority. Cobalt's modular resource ABI wrapper needs
// them; the shim answers ENOSYS.
#ifndef NX_RLIMIT_DEFINED
#define NX_RLIMIT_DEFINED
typedef unsigned long nx_rlim_t;
struct rlimit {
  nx_rlim_t rlim_cur;
  nx_rlim_t rlim_max;
};
#endif
#ifndef RLIM_INFINITY
#define RLIM_INFINITY (~(unsigned long)0)
#endif
#ifndef RLIM_SAVED_CUR
#define RLIM_SAVED_CUR RLIM_INFINITY
#endif
#ifndef RLIM_SAVED_MAX
#define RLIM_SAVED_MAX RLIM_INFINITY
#endif
#ifndef PRIO_PROCESS
#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2
#endif
#ifdef __cplusplus
extern "C" {
#endif
int getrlimit(int resource, struct rlimit* rlim);
int setrlimit(int resource, const struct rlimit* rlim);
int getpriority(int which, int who);
int setpriority(int which, int who, int prio);
#ifdef __cplusplus
}
#endif
// Linux resource limits (newlib has none) and POSIX scheduling extensions that
// newlib omits. Cobalt's modular ABI wrappers reference the whole family, so
// define it in one go rather than one error at a time.
#ifndef RLIMIT_CPU
#define RLIMIT_CPU 0
#define RLIMIT_FSIZE 1
#define RLIMIT_DATA 2
#define RLIMIT_STACK 3
#define RLIMIT_CORE 4
#define RLIMIT_RSS 5
#define RLIMIT_NPROC 6
#define RLIMIT_NOFILE 7
#define RLIMIT_MEMLOCK 8
#define RLIMIT_AS 9
#define RLIMIT_LOCKS 10
#define RLIMIT_SIGPENDING 11
#define RLIMIT_MSGQUEUE 12
#define RLIMIT_NICE 13
#define RLIMIT_RTPRIO 14
#endif

#ifndef SCHED_BATCH
#define SCHED_BATCH 3
#endif
#ifndef SCHED_IDLE
#define SCHED_IDLE 5
#endif
#ifndef NX_CPU_SETSIZE
#define NX_CPU_SETSIZE 64
#endif
#ifndef CPU_ZERO
typedef struct {
  unsigned long __bits[(NX_CPU_SETSIZE + 63) / 64];
} cpu_set_t;
#define CPU_ZERO(set)   do { for (int i = 0; i < (NX_CPU_SETSIZE + 63) / 64; ++i) (set)->__bits[i] = 0; } while (0)
#define CPU_SET(cpu, set) ((set)->__bits[(cpu) / 64] |= (1UL << ((cpu) % 64)))
#define CPU_CLR(cpu, set) ((set)->__bits[(cpu) / 64] &= ~(1UL << ((cpu) % 64)))
#define CPU_ISSET(cpu, set) (((set)->__bits[(cpu) / 64] >> ((cpu) % 64)) & 1UL)
#endif

#ifdef __cplusplus
extern "C" {
#endif
int sched_setscheduler(pid_t pid, int policy, const struct sched_param* param);
int sched_getscheduler(pid_t pid);
int sched_setparam(pid_t pid, const struct sched_param* param);
int sched_getparam(pid_t pid, struct sched_param* param);
int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t* mask);
int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t* mask);
#ifdef __cplusplus
}
#endif
// libnx's <net/if.h> has these commented out; Cobalt's socket ABI wrapper uses
// if_nametoindex(). Switch has no interface index, so it returns 0 (failure).
#ifdef __cplusplus
extern "C" {
#endif
unsigned int if_nametoindex(const char* ifname);
#ifdef __cplusplus
}
#endif
#endif
