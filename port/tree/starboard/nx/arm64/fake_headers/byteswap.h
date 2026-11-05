// Nintendo Switch: newlib has no <byteswap.h> (glibc's bswap_* helpers).
// protobuf's port.h includes it on the Linux path, so back it with GCC builtins.
#ifndef FAKE_BYTESWAP_H_
#define FAKE_BYTESWAP_H_

#include <stdint.h>

#define bswap_16(x) __builtin_bswap16((uint16_t)(x))
#define bswap_32(x) __builtin_bswap32((uint32_t)(x))
#define bswap_64(x) __builtin_bswap64((uint64_t)(x))

#endif  // FAKE_BYTESWAP_H_
