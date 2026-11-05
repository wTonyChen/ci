// Nintendo Switch: neither newlib nor libnx ships <sys/un.h>. perfetto's
// unix_socket.cc includes it for struct sockaddr_un (AF_UNIX sockets are not
// usable on Horizon anyway — socketpair returns ENOSYS — so this is a
// compile-time surface only).
#ifndef FAKE_SYS_UN_H_
#define FAKE_SYS_UN_H_

#include <sys/socket.h>

#ifndef AF_UNIX
#define AF_UNIX 1
#endif
#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif
#ifndef AF_UNIX_PATH_MAX
#define AF_UNIX_PATH_MAX 108
#endif

struct sockaddr_un {
  sa_family_t sun_family;
  char sun_path[108];
};

#endif  // FAKE_SYS_UN_H_
