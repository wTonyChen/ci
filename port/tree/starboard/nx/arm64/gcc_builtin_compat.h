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
#endif
