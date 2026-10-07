// Minimal <link.h> for Nintendo Switch (libnx / newlib has no dynamic linker).
//
// Chromium's base/third_party/symbolize uses dl_iterate_phdr() to walk loaded
// objects. On Switch there is nothing to walk, so this exists purely so that the
// (unused) symbolizer compiles; symbolization is a no-op here.
// TODO(chromium-nx): drop the //starboard/linux/shared dependency and this shim
// once the nx platform has its own windowing/EGL implementation.
#ifndef CHROMIUM_NX_LINK_H_
#define CHROMIUM_NX_LINK_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dl_phdr_info {
  uintptr_t dlpi_addr;
  const char* dlpi_name;
  const void* dlpi_phdr;
  uint16_t dlpi_phnum;
};

typedef int (*dl_iterate_phdr_callback)(struct dl_phdr_info* info,
                                        size_t size,
                                        void* data);
int dl_iterate_phdr(dl_iterate_phdr_callback callback, void* data);

#ifdef __cplusplus
}
#endif

#endif  // CHROMIUM_NX_LINK_H_
