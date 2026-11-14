#ifndef FAKE_LINUX_AUXVEC_H_
#define FAKE_LINUX_AUXVEC_H_

// Horizon has no ELF auxiliary vector, but V8's Linux branch includes this
// header for the AT_* constants (cpu.cc wants AT_HWCAP / AT_HWCAP2). Keep the
// generic ELF values so any reader at least compiles; getauxval() in
// shims/nx_extra_shim.cc returns 0 for everything.
#ifndef AT_NULL
#define AT_NULL 0
#endif
#ifndef AT_IGNORE
#define AT_IGNORE 1
#endif
#ifndef AT_EXECFD
#define AT_EXECFD 2
#endif
#ifndef AT_PHDR
#define AT_PHDR 3
#endif
#ifndef AT_PHENT
#define AT_PHENT 4
#endif
#ifndef AT_PHNUM
#define AT_PHNUM 5
#endif
#ifndef AT_PAGESZ
#define AT_PAGESZ 6
#endif
#ifndef AT_BASE
#define AT_BASE 7
#endif
#ifndef AT_FLAGS
#define AT_FLAGS 8
#endif
#ifndef AT_ENTRY
#define AT_ENTRY 9
#endif
#ifndef AT_UID
#define AT_UID 11
#endif
#ifndef AT_EUID
#define AT_EUID 12
#endif
#ifndef AT_GID
#define AT_GID 13
#endif
#ifndef AT_EGID
#define AT_EGID 14
#endif
#ifndef AT_PLATFORM
#define AT_PLATFORM 15
#endif
#ifndef AT_HWCAP
#define AT_HWCAP 16
#endif
#ifndef AT_CLKTCK
#define AT_CLKTCK 17
#endif
#ifndef AT_SECURE
#define AT_SECURE 23
#endif
#ifndef AT_RANDOM
#define AT_RANDOM 25
#endif
#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif
#ifndef AT_EXECFN
#define AT_EXECFN 31
#endif

#endif
