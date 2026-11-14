// posix_extra.cc —— //base 里少数 POSIX 调用在 newlib/libnx 上没有实现。
//
// 为什么单独一个文件：posix_shim.cc 已经很长了，这里只放"补洞"的小函数，
// 全部是 libc 级替换（不涉及 Chromium 语义），而且都是最保守的实现：
//   * pwrite/pread 用 lseek+write/read 模拟（SD 卡上的 devoptab 本身就是顺序 IO，
//     base::File 只在同一线程里用 offset 读写）；
//   * fdatasync 直接返回成功（Switch 没有掉电保护语义，fsync 也只是刷缓存）；
//   * creat 走 open，geteuid/getegid 恒为 root（libnx 进程没有 uid 概念）。
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <switch.h>

extern "C" {

// 新树里 base::File 的 WriteAndCheck/ReadAndCheck 走 pwrite/pread。
ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
  const off_t saved = lseek(fd, 0, SEEK_CUR);
  if (saved < 0)
    return -1;
  if (lseek(fd, offset, SEEK_SET) < 0)
    return -1;
  const ssize_t r = write(fd, buf, count);
  const int e = errno;
  lseek(fd, saved, SEEK_SET);
  errno = e;
  return r;
}

int fdatasync(int /*fd*/) {
  return 0;
}

int creat(const char* path, mode_t mode) {
  return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

uid_t geteuid(void) {
  return 0;
}

gid_t getegid(void) {
  return 0;
}

}  // extern "C"

// ---- 第二批：//base 的 POSIX 文件/线程路径会用到、但 newlib 里根本没有的调用 ----
#include <sched.h>
#include <string.h>
#include <sys/time.h>

namespace {
FsFileSystem* SdFs() {
  static FsFileSystem fs;
  static int state = 0;  // 0=未初始化 1=好 -1=坏
  if (state == 0)
    state = R_SUCCEEDED(fsOpenSdCardFileSystem(&fs)) ? 1 : -1;
  return state == 1 ? &fs : nullptr;
}
}  // namespace

extern "C" {

// newlib 没有 mkdir；Switch 上真正要建目录的地方都是 sdmc:。
int mkdir(const char* path, mode_t /*mode*/) {
  FsFileSystem* fs = SdFs();
  if (!fs) {
    errno = ENOSYS;
    return -1;
  }
  const char* rel = path;
  if (strncmp(rel, "sdmc:", 5) == 0)
    rel = rel + 5;
  if (R_FAILED(fsFsCreateDirectory(fs, rel))) {
    errno = EIO;
    return -1;
  }
  return 0;
}

int ftruncate(int /*fd*/, off_t /*length*/) {
  // devoptab 没有 truncate 钩子；探针只追加写，不需要真的截断。
  return 0;
}

int futimes(int /*fd*/, const struct timeval /*tv*/[2]) {
  return 0;
}

int futimens(int /*fd*/, const struct timespec /*ts*/[2]) {
  return 0;
}}  // extern "C"
