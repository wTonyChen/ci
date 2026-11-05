// Nintendo Switch: newlib has no <sys/random.h> and Horizon has no getrandom()
// syscall, but libnx exposes the CSRNG through randomGet().
#ifndef FAKE_SYS_RANDOM_H_
#define FAKE_SYS_RANDOM_H_

#include <stddef.h>
#include <sys/types.h>

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x0001
#endif
#ifndef GRND_RANDOM
#define GRND_RANDOM 0x0002
#endif

#ifdef __cplusplus
extern "C" {
#endif

// libnx's CSRNG (switch/kernel/random.h); declared here directly so this header
// does not have to pull all of <switch.h>.
void randomGet(void* buf, size_t len);

static inline ssize_t getrandom(void* buf, size_t buflen, unsigned int flags) {
  (void)flags;
  if (buflen == 0) {
    return 0;
  }
  randomGet(buf, buflen);
  return (ssize_t)buflen;
}

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_RANDOM_H_
