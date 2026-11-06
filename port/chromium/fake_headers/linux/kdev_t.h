// Nintendo Switch: newlib has no <linux/kdev_t.h>. It only exists to expose the
// major()/minor() helpers to kernel-space code; userspace code is expected to
// include <sys/sysmacros.h>, which we provide.
#ifndef FAKE_LINUX_KDEV_T_H_
#define FAKE_LINUX_KDEV_T_H_

#include <sys/sysmacros.h>

/* Kernel spelling of the same helpers (base/files/drive_info_posix.cc uses
   MAJOR()/MINOR(), which come from <linux/kdev_t.h> on Linux). */
#ifndef MAJOR
#define MAJOR(dev) gnu_dev_major(dev)
#endif
#ifndef MINOR
#define MINOR(dev) gnu_dev_minor(dev)
#endif
#ifndef MKDEV
#define MKDEV(major, minor) gnu_dev_makedev(major, minor)
#endif

#endif  // FAKE_LINUX_KDEV_T_H_
