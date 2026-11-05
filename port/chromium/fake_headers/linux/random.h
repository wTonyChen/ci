// Nintendo Switch: newlib has no <linux/random.h>. BoringSSL's urandom.cc
// includes it for the getrandom(2) flags; the call itself goes through our
// <sys/random.h> wrapper.
#ifndef FAKE_LINUX_RANDOM_H_
#define FAKE_LINUX_RANDOM_H_

#include <sys/random.h>

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x0001
#endif
#ifndef GRND_RANDOM
#define GRND_RANDOM 0x0002
#endif

#endif  // FAKE_LINUX_RANDOM_H_
