// Nintendo Switch: newlib has no <sys/sendfile.h>.
#ifndef FAKE_SYS_SENDFILE_H_
#define FAKE_SYS_SENDFILE_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif
static inline ssize_t sendfile(int out_fd, int in_fd, off_t* offset,
                               size_t count) {
  (void)out_fd;
  (void)in_fd;
  (void)offset;
  (void)count;
  return -1;
}
#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_SENDFILE_H_
