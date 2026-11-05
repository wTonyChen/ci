// Nintendo Switch: devkitA64's newlib has NO <sys/ioctl.h> at all, and Horizon
// has no tty/ioctl. perfetto's progress_reporter.cc calls ioctl(TIOCGWINSZ).
#ifndef FAKE_SYS_IOCTL_H_
#define FAKE_SYS_IOCTL_H_

#ifndef TIOCGWINSZ
struct winsize {
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
};
#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414
#endif

#ifdef __cplusplus
extern "C" {
#endif
static inline int ioctl(int fd, unsigned long request, ...) {
  (void)fd;
  (void)request;
  return -1;
}
#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_IOCTL_H_
