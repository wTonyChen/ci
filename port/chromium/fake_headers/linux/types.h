// Nintendo Switch: newlib has no <linux/types.h>. perfetto's futex.cc includes
// it for the __u32/__s64 style typedefs; provide the small subset.
#ifndef FAKE_LINUX_TYPES_H_
#define FAKE_LINUX_TYPES_H_

#include <stddef.h>
#include <stdint.h>

typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;
typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;
typedef int64_t __s64;

typedef __u32 __kernel_uid32_t;
typedef __u32 __kernel_gid32_t;

#endif  // FAKE_LINUX_TYPES_H_
