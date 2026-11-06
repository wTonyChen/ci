// Nintendo Switch: newlib has <sys/statvfs.h> but neither <sys/statfs.h> nor
// <sys/vfs.h> (they are glibc's). base/process/process_linux.cc calls statfs()
// and checks f_type against the magic numbers.
//
// f_type reports 0, which matches none of the pseudo-fs magics — correct for
// Horizon, where these /proc and /sys checks are meaningless anyway.
#ifndef FAKE_SYS_VFS_H_
#define FAKE_SYS_VFS_H_

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

typedef long int __fsword_t;

struct statfs {
  __fsword_t f_type;
  __fsword_t f_bsize;
  fsblkcnt_t f_blocks;
  fsblkcnt_t f_bfree;
  fsblkcnt_t f_bavail;
  fsfilcnt_t f_files;
  fsfilcnt_t f_ffree;
  __fsword_t f_fsid;
  __fsword_t f_namelen;
  __fsword_t f_frsize;
  __fsword_t f_flags;
  __fsword_t f_spare[4];
};

#ifdef __cplusplus
extern "C" {
#endif

static inline int statfs(const char* path, struct statfs* buf) {
  (void)path;
  (void)buf;
  errno = ENOSYS;
  return -1;
}

static inline int fstatfs(int fd, struct statfs* buf) {
  (void)fd;
  (void)buf;
  errno = ENOSYS;
  return -1;
}

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_VFS_H_
