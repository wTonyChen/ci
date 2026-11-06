// Force-included into every target translation unit of the Switch build
// (via -include from //:switch_config). Ported from the Cobalt port's
// gcc_builtin_compat.h, which was validated over ~20 CI rounds.
#ifndef SWITCH_COMPAT_H_
#define SWITCH_COMPAT_H_

// POSIX <limits.h> extras that newlib does not expose (symbolize.cc uses it).
#include <limits.h>
// Linux large-file / BSD compatibility names that newlib does not define.
// aarch64's off_t is already 64-bit, so the *64 variants are plain aliases.
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif
#ifndef ftruncate64
#define ftruncate64 ftruncate
#endif
#ifndef lseek64
#define lseek64 lseek
#endif
#ifndef stat64
#define stat64 stat
#endif
#ifndef fstat64
#define fstat64 fstat
#endif
#ifndef open64
#define open64 open
#endif
#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 16384
#endif
#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)(SIZE_MAX >> 1))
#endif


// HOST CLANG: inert. Rust bindgen is invoked with the *target* cflags, so it
// also gets `-include switch_compat.h`; none of the newlib-facing declarations
// below make sense on the x86_64 host. The target compiler is clang too (plan
// A), so discriminate by architecture: only aarch64 sees the body.
#if defined(__clang__) && !defined(__aarch64__)
#define NX_HOST_CLANG 1
#endif

#if defined(NX_HOST_CLANG)

#else  // aarch64 target (any compiler)

#ifndef _POSIX_THREAD_PRIO_INHERIT
#define _POSIX_THREAD_PRIO_INHERIT 1
#endif
#ifndef _POSIX_THREAD_PRIO_PROTECT
#define _POSIX_THREAD_PRIO_PROTECT 1
#endif
#ifndef _UNIX98_THREAD_MUTEX_ATTRIBUTES
#define _UNIX98_THREAD_MUTEX_ATTRIBUTES 1
#endif


// newlib hides several POSIX thread APIs behind feature-test macros. Chromium's
// base/synchronization/lock_impl_posix.cc uses the mutex protocol API
// (PTHREAD_PRIO_INHERIT) inside DCHECK, and base/threading uses mutex types.
// Enabling them here (before <pthread.h> is pulled in) makes newlib declare them.
// Chromium sources rely on libc++ transitive includes (e.g. proc_maps_linux.cc
// calls strlen() after only including <string>). The devkitA64 libstdc++ headers
// do not pull the C headers transitively, so force in the ones Chromium assumes.
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>





// Linux SO_PEERCRED / struct ucred (perfetto's unix_socket.cc). glibc puts
// these in <sys/socket.h>; newlib/libnx have neither.
#include <sys/types.h>
#ifndef SO_PEERCRED
#define SO_PEERCRED 17
#endif
#ifndef NX_UCRED_DEFINED
#define NX_UCRED_DEFINED
struct ucred {
  pid_t pid;
  uid_t uid;
  gid_t gid;
};
#endif


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
// Host clang (Rust bindgen) also receives this -include; it must not pull
// newlib/devkitA64 headers, so only the real GCC target includes anything.
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

// NOTE: ElfW()/ELFMAG are deliberately NOT defined here. <elf.h> defines a lot
// of ELF* macros (ELFMAG0, PT_LOAD, SHT_NOTE, ...) and perfetto defines its own
// `constexpr auto ELFMAG0` etc. in trace_processor/util/elf/elf.h — force-
// including <elf.h> into every TU turns those into "expected unqualified-id
// before numeric constant". They now live in fake_headers/link.h, which only
// the partition_alloc stack-trace TU includes.
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

// Dynamic cpu_set_t helpers (glibc-specific): our cpu_set_t is a fixed 64-bit
// mask, so the "alloc" family just maps onto it.
#ifndef CPU_COUNT
static inline int nx_cpu_count(const cpu_set_t* set) {
  int n = 0;
  for (int i = 0; i < (NX_CPU_SETSIZE + 63) / 64; ++i) {
    n += __builtin_popcountl(set->__bits[i]);
  }
  return n;
}
#define CPU_COUNT(set) nx_cpu_count(set)
#endif
#ifndef CPU_ALLOC_SIZE
#define CPU_ALLOC_SIZE(count) ((size_t)sizeof(cpu_set_t))
#endif
#ifndef CPU_ALLOC
#define CPU_ALLOC(count) ((cpu_set_t*)calloc(1, sizeof(cpu_set_t)))
#endif
#ifndef CPU_FREE
#define CPU_FREE(set) free(set)
#endif
#ifndef CPU_COUNT_S
#define CPU_COUNT_S(setsize, set) CPU_COUNT(set)
#endif

// Linux SCM credentials (base/posix/unix_domain_socket.cc). AF_UNIX is inert on
// Horizon but the constants have to exist.
#ifndef SO_PASSCRED
#define SO_PASSCRED 16
#endif
#ifndef SCM_CREDENTIALS
#define SCM_CREDENTIALS 2
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
// libnx's <sys/socket.h> is missing two Linux message flags and its <net/if.h>
// has if_indextoname() commented out; Cobalt's socket ABI wrapper uses all three.
#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0x800
#endif
#ifndef MSG_MORE
#define MSG_MORE 0x8000
#endif
#ifdef __cplusplus
extern "C" {
#endif
char* if_indextoname(unsigned int ifindex, char* ifname);
#ifdef __cplusplus
}
#endif
// newlib has no UTIME_NOW/UTIME_OMIT (glibc values); Cobalt's stat wrapper
// translates them.
#ifndef UTIME_NOW
#define UTIME_NOW ((1l << 30) - 1l)
#endif
#ifndef UTIME_OMIT
#define UTIME_OMIT ((1l << 30) - 2l)
#endif
// newlib hides clock_nanosleep() behind _POSIX_CLOCK_SELECTION (which it does
// not support). Cobalt's time ABI wrapper needs it; the shim builds it on
// nanosleep().
#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 1
#endif
#ifdef __cplusplus
extern "C" {
#endif
int clock_nanosleep(clockid_t clock_id, int flags,
                    const struct timespec* rqtp, struct timespec* rmtp);
#ifdef __cplusplus
}
#endif
// newlib only spells out a couple of POSIX clock ids; Linux code paths in
// Chromium/perfetto reference the whole set. Values are the Linux ones.
#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID 2
#endif
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif
#ifndef CLOCK_REALTIME_COARSE
#define CLOCK_REALTIME_COARSE 5
#endif
#ifndef CLOCK_MONOTONIC_COARSE
#define CLOCK_MONOTONIC_COARSE 6
#endif
#ifndef CLOCK_BOOTTIME
#define CLOCK_BOOTTIME 7
#endif
#ifndef CLOCK_REALTIME_ALARM
#define CLOCK_REALTIME_ALARM 8
#endif
#ifndef CLOCK_BOOTTIME_ALARM
#define CLOCK_BOOTTIME_ALARM 9
#endif
// glibc exports these from <errno.h>; base/process/set_process_title.cc uses
// them AND assigns to them, so they must be char* (assignable), not arrays.
// A per-TU static is enough for the port: nothing reads them across TUs.
static char program_invocation_short_name_storage[] = "chromium";
static char program_invocation_name_storage[] = "chromium";
static char* program_invocation_short_name = program_invocation_short_name_storage;
static char* program_invocation_name = program_invocation_name_storage;

#endif  // !__clang__
#endif  // SWITCH_COMPAT_H_