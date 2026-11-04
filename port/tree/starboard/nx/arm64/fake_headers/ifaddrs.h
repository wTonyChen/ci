// Nintendo Switch: newlib has no <ifaddrs.h> and Horizon has no interface list
// API. Cobalt's modular socket ABI wrapper only needs the declarations; the
// shim reports "not supported".
#ifndef FAKE_IFADDRS_H_
#define FAKE_IFADDRS_H_

#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ifaddrs {
  struct ifaddrs* ifa_next;
  char* ifa_name;
  unsigned int ifa_flags;
  struct sockaddr* ifa_addr;
  struct sockaddr* ifa_netmask;
  union {
    struct sockaddr* ifu_broadaddr;
    struct sockaddr* ifu_dstaddr;
  } ifa_ifu;
  void* ifa_data;
};

#define ifa_broadaddr ifa_ifu.ifu_broadaddr
#define ifa_dstaddr ifa_ifu.ifu_dstaddr

int getifaddrs(struct ifaddrs** ifap);
void freeifaddrs(struct ifaddrs* ifa);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_IFADDRS_H_
