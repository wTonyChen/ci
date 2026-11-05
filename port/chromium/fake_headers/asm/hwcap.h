// Nintendo Switch: devkitA64's newlib has no <asm/hwcap.h>. Chromium (zlib,
// partition_alloc, base) reads the aarch64 HWCAP_* bits; provide the standard
// Linux values so the code compiles and the capability checks stay consistent.
#ifndef FAKE_ASM_HWCAP_H_
#define FAKE_ASM_HWCAP_H_

#ifndef HWCAP_FP
#define HWCAP_FP (1 << 0)
#endif
#ifndef HWCAP_ASIMD
#define HWCAP_ASIMD (1 << 1)
#endif
#ifndef HWCAP_EVTSTRM
#define HWCAP_EVTSTRM (1 << 2)
#endif
#ifndef HWCAP_AES
#define HWCAP_AES (1 << 3)
#endif
#ifndef HWCAP_PMULL
#define HWCAP_PMULL (1 << 4)
#endif
#ifndef HWCAP_SHA1
#define HWCAP_SHA1 (1 << 5)
#endif
#ifndef HWCAP_SHA2
#define HWCAP_SHA2 (1 << 6)
#endif
#ifndef HWCAP_CRC32
#define HWCAP_CRC32 (1 << 7)
#endif
#ifndef HWCAP_ATOMICS
#define HWCAP_ATOMICS (1 << 8)
#endif
#ifndef HWCAP_FPHP
#define HWCAP_FPHP (1 << 9)
#endif
#ifndef HWCAP_ASIMDHP
#define HWCAP_ASIMDHP (1 << 10)
#endif
#ifndef HWCAP_CPUID
#define HWCAP_CPUID (1 << 11)
#endif
#ifndef HWCAP_ASIMDRDM
#define HWCAP_ASIMDRDM (1 << 12)
#endif
#ifndef HWCAP_JSCVT
#define HWCAP_JSCVT (1 << 13)
#endif
#ifndef HWCAP_FCMA
#define HWCAP_FCMA (1 << 14)
#endif
#ifndef HWCAP_LRCPC
#define HWCAP_LRCPC (1 << 15)
#endif
#ifndef HWCAP_DCPOP
#define HWCAP_DCPOP (1 << 16)
#endif
#ifndef HWCAP_SHA3
#define HWCAP_SHA3 (1 << 17)
#endif
#ifndef HWCAP_SM3
#define HWCAP_SM3 (1 << 18)
#endif
#ifndef HWCAP_SM4
#define HWCAP_SM4 (1 << 19)
#endif
#ifndef HWCAP_ASIMDDP
#define HWCAP_ASIMDDP (1 << 20)
#endif
#ifndef HWCAP_SHA512
#define HWCAP_SHA512 (1 << 21)
#endif

/* HWCAP2 bits */
#ifndef HWCAP2_DCPODP
#define HWCAP2_DCPODP (1 << 0)
#endif
#ifndef HWCAP2_SVE2
#define HWCAP2_SVE2 (1 << 1)
#endif
#ifndef HWCAP2_BTI
#define HWCAP2_BTI (1 << 2)
#endif
#ifndef HWCAP2_MTE
#define HWCAP2_MTE (1 << 3)
#endif

#endif  // FAKE_ASM_HWCAP_H_
