// Nintendo Switch: newlib has no <linux/magic.h>. base/process/process_linux.cc
// compares statfs f_type against these to recognise pseudo filesystems; the
// values are the standard Linux ones.
#ifndef FAKE_LINUX_MAGIC_H_
#define FAKE_LINUX_MAGIC_H_

#define AFFS_SUPER_MAGIC 0xadff
#define BINFMTFS_MAGIC 0x42494e4d
#define CGROUP_SUPER_MAGIC 0x27e0eb
#define CGROUP2_SUPER_MAGIC 0x63677270
#define DEBUGFS_MAGIC 0x64626720
#define DEVMEM_MAGIC 0x454d444d
#define FUSE_SUPER_MAGIC 0x65735546
#define FUTEXFS_SUPER_MAGIC 0xbad1dea
#define HUGETLBFS_MAGIC 0x958458f6
#define OVERLAYFS_SUPER_MAGIC 0x794c7630
#define PROC_SUPER_MAGIC 0x9fa0
#define RAMFS_MAGIC 0x858458f6
#define SECURITYFS_MAGIC 0x73636673
#define SELINUX_MAGIC 0xf97cff8c
#define SMB_SUPER_MAGIC 0x517b
#define SYSFS_MAGIC 0x62656572
#define TMPFS_MAGIC 0x01021994
#define TRACEFS_MAGIC 0x74726163
#define USBDEVICE_SUPER_MAGIC 0x9fa2

#endif  // FAKE_LINUX_MAGIC_H_
