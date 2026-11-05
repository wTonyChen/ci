// Nintendo Switch: newlib has no <sys/random.h>. Cobalt's modular getrandom
// ABI wrapper and abseil both want the glibc-style getrandom() declaration;
// the shim backs it with libnx's randomGet().
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

ssize_t getrandom(void* buf, size_t buflen, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_RANDOM_H_
