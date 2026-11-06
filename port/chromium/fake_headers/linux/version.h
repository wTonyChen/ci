// Nintendo Switch: newlib has no <linux/version.h>. partition_alloc's
// tagging.cc compares LINUX_VERSION_CODE against kernel versions to decide
// whether MTE / pkeys are available. Report an old kernel deliberately: Horizon
// has none of those features, and a low version keeps those code paths off.
#ifndef FAKE_LINUX_VERSION_H_
#define FAKE_LINUX_VERSION_H_

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif
#ifndef LINUX_VERSION_CODE
#define LINUX_VERSION_CODE KERNEL_VERSION(4, 14, 0)
#endif

#endif  // FAKE_LINUX_VERSION_H_
