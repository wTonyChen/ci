// Nintendo Switch: newlib has no <ucontext.h>. abseil includes the POSIX
// spelling in some translation units (<sys/ucontext.h> in others).
#ifndef FAKE_UCONTEXT_H_
#define FAKE_UCONTEXT_H_

#include <sys/ucontext.h>

#endif  // FAKE_UCONTEXT_H_
