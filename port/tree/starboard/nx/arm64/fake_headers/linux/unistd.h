// Nintendo Switch: newlib has no <linux/unistd.h>. abseil's direct_mmap.h
// includes it for the SYS_* numbers on the Linux path; our fake <sys/syscall.h>
// already carries the aarch64 numbers we need.
#ifndef FAKE_LINUX_UNISTD_H_
#define FAKE_LINUX_UNISTD_H_

#include <sys/syscall.h>

#endif  // FAKE_LINUX_UNISTD_H_
