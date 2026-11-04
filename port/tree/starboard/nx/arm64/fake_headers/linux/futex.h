// Nintendo Switch: newlib has no <linux/futex.h>. partition_alloc's
// spinning_mutex.cc takes the HAS_LINUX_KERNEL path because we build with
// target_os="linux"; provide the constants so it compiles. The futex syscall
// itself goes through the syscall() shim and currently answers ENOSYS.
#ifndef FAKE_LINUX_FUTEX_H_
#define FAKE_LINUX_FUTEX_H_

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_FD 2
#define FUTEX_REQUEUE 3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP 5
#define FUTEX_LOCK_PI 6
#define FUTEX_UNLOCK_PI 7
#define FUTEX_TRYLOCK_PI 8
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10

#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_CLOCK_REALTIME 256
#define FUTEX_CMD_MASK (~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME))

#define FUTEX_WAIT_PRIVATE (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)

#endif  // FAKE_LINUX_FUTEX_H_
