// Nintendo Switch: newlib has no <sys/ifunc.h> (glibc's aarch64 IFUNC helper
// header). partition_alloc's aarch64_support.h includes it whenever the target
// is Linux/Android to read the BTI/MTE capabilities, so provide the two symbols
// it actually uses.
#ifndef FAKE_SYS_IFUNC_H_
#define FAKE_SYS_IFUNC_H_

struct __ifunc_arg_t {
  unsigned long _size;
  unsigned long _hwcap;
  unsigned long _hwcap2;
};

#define _IFUNC_ARG_HWCAP (1ULL << 62)

#endif  // FAKE_SYS_IFUNC_H_
