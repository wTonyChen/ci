// Nintendo Switch: newlib has no <sys/sysmacros.h>.
// base/process/process_metrics_linux.cc uses major()/minor() on st_dev.
// NOTE: devkitA64's dev_t is 32-bit, so use the classic 8/8 split and do the
// arithmetic in uint64_t to avoid shift-count warnings.
#ifndef FAKE_SYS_SYSMACROS_H_
#define FAKE_SYS_SYSMACROS_H_

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline unsigned int gnu_dev_major(dev_t dev) {
  return (unsigned int)(((uint64_t)(unsigned int)dev >> 8) & 0xffu);
}

static inline unsigned int gnu_dev_minor(dev_t dev) {
  return (unsigned int)((uint64_t)(unsigned int)dev & 0xffu);
}

static inline dev_t gnu_dev_makedev(unsigned int major, unsigned int minor) {
  return (dev_t)((((uint64_t)major & 0xffu) << 8) | ((uint64_t)minor & 0xffu));
}

#ifdef __cplusplus
}
#endif

#ifndef major
#define major(dev) gnu_dev_major(dev)
#endif
#ifndef minor
#define minor(dev) gnu_dev_minor(dev)
#endif
#ifndef makedev
#define makedev(maj, min) gnu_dev_makedev(maj, min)
#endif

#endif  // FAKE_SYS_SYSMACROS_H_
