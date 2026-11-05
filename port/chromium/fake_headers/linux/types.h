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

// asm-generic/posix_types.h subset (perfetto's futex.cc needs __kernel_long_t).
typedef long __kernel_long_t;
typedef unsigned long __kernel_ulong_t;
typedef __kernel_long_t __kernel_off_t;
typedef long long __kernel_loff_t;
typedef __kernel_long_t __kernel_time_t;
typedef long long __kernel_time64_t;
typedef __kernel_long_t __kernel_clock_t;
typedef int __kernel_pid_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef __kernel_long_t __kernel_ssize_t;
typedef __kernel_long_t __kernel_ptrdiff_t;
typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;
typedef unsigned long __kernel_size_t;
typedef __kernel_ulong_t __kernel_ino_t;
typedef unsigned int __kernel_mode_t;
typedef int __kernel_daddr_t;
typedef char* __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;
typedef unsigned short __kernel_old_dev_t;
typedef __u32 __kernel_uid32_t;
typedef __u32 __kernel_gid32_t;

#endif  // FAKE_LINUX_TYPES_H_
