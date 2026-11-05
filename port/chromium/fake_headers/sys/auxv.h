#ifndef FAKE_SYS_AUXV_H_
#define FAKE_SYS_AUXV_H_

// newlib's <sys/auxv.h> (when present at all) only defines AT_HWCAP. Chromium
// partition_alloc reads AT_HWCAP2 unconditionally. Values are the generic
// ELF/aarch64 ones from <elf.h>.
#ifndef AT_HWCAP
#define AT_HWCAP 16
#endif
#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif

#ifdef __cplusplus
extern "C" {
#endif
unsigned long getauxval(unsigned long type);
#ifdef __cplusplus
}
#endif
#endif
