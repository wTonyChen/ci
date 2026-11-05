// Nintendo Switch / devkitA64: newlib's <termios.h> just does
//   #include <sys/termios.h>
// but that file is MISSING from the devkitA64 newlib install (only
// machine/termios.h with __MAX_BAUD exists). So provide the POSIX terminal
// interface surface here; the calls themselves are inline stubs because Horizon
// has no tty. perfetto's progress_reporter.cc only needs the struct + decls.
#ifndef FAKE_SYS_TERMIOS_H_
#define FAKE_SYS_TERMIOS_H_

#include <sys/types.h>

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

#define NCCS 32

struct termios {
  tcflag_t c_iflag;
  tcflag_t c_oflag;
  tcflag_t c_cflag;
  tcflag_t c_lflag;
  cc_t c_line;
  cc_t c_cc[NCCS];
  speed_t c_ispeed;
  speed_t c_ospeed;
};

/* c_cc indices (Linux values) */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11

/* c_iflag */
#define IGNBRK 0000001
#define BRKINT 0000002
#define IGNPAR 0000004
#define PARMRK 0000010
#define INPCK 0000020
#define ISTRIP 0000040
#define INLCR 0000100
#define IGNCR 0000200
#define ICRNL 0000400
#define IXON 0002000
#define IXOFF 0010000

/* c_oflag */
#define OPOST 0000001
#define ONLCR 0000004

/* c_cflag */
#define CSIZE 0000060
#define CS8 0000060
#define CREAD 0000200
#define CLOCAL 0004000

/* c_lflag */
#define ISIG 0000001
#define ICANON 0000002
#define ECHO 0000010
#define ECHOE 0000020
#define ECHOK 0000040
#define ECHONL 0000100
#define NOFLSH 0000200
#define TOSTOP 0000400
#define IEXTEN 0100000

/* tcsetattr actions */
#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#ifdef __cplusplus
extern "C" {
#endif
static inline int tcgetattr(int fd, struct termios* tio) {
  (void)fd;
  (void)tio;
  return -1;
}
static inline int tcsetattr(int fd, int optional_actions,
                            const struct termios* tio) {
  (void)fd;
  (void)optional_actions;
  (void)tio;
  return -1;
}
#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_TERMIOS_H_
