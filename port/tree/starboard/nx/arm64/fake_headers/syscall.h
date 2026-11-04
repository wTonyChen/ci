// Nintendo Switch: newlib has no <syscall.h> (only the Linux kernel uapi one).
// abseil's direct_mmap.h includes <syscall.h> and uses SYS_mmap/SYS_munmap on
// the Linux path, so map it onto our fake <sys/syscall.h>.
#ifndef FAKE_SYSCALL_H_
#define FAKE_SYSCALL_H_

#include <sys/syscall.h>

#endif  // FAKE_SYSCALL_H_
