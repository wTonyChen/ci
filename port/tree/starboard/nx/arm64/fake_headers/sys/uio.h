// Nintendo Switch: newlib has no <sys/uio.h>. Cobalt's modular uio ABI wrapper
// needs the vector I/O declarations; the shim implements them on top of
// read()/write() (see shims/posix_shim.cc).
#ifndef FAKE_SYS_UIO_H_
#define FAKE_SYS_UIO_H_

#include <stddef.h>
#include <sys/types.h>

// libnx's <sys/socket.h> already provides struct iovec via <sys/_iovec.h>.
#ifndef _SYS__IOVEC_H_
struct iovec {
  void* iov_base;
  size_t iov_len;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

ssize_t readv(int fd, const struct iovec* iov, int iovcnt);
ssize_t writev(int fd, const struct iovec* iov, int iovcnt);
ssize_t preadv(int fd, const struct iovec* iov, int iovcnt, off_t offset);
ssize_t pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_UIO_H_
