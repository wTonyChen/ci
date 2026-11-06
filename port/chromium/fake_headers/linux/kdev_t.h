// Nintendo Switch: newlib has no <linux/kdev_t.h>. It only exists to expose the
// major()/minor() helpers to kernel-space code; userspace code is expected to
// include <sys/sysmacros.h>, which we provide.
#ifndef FAKE_LINUX_KDEV_T_H_
#define FAKE_LINUX_KDEV_T_H_

#include <sys/sysmacros.h>

#endif  // FAKE_LINUX_KDEV_T_H_
