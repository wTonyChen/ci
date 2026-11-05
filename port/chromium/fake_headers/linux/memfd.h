// Nintendo Switch: newlib has no <linux/memfd.h>; provide the constants only
// (memfd_create itself is not implemented on Horizon).
#ifndef FAKE_LINUX_MEMFD_H_
#define FAKE_LINUX_MEMFD_H_

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif

#endif  // FAKE_LINUX_MEMFD_H_
