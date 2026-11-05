// Nintendo Switch: newlib has <sys/features.h> but no top-level <features.h>
// (that is glibc's; partition_alloc includes it on the Linux path).
//
// IMPORTANT: this header is also seen by the *host* clang of the Rust bindgen
// step, which has no newlib include path — so only forward to sys/features.h
// for the real (GCC) target compile. glibc-detection branches stay off, which
// is what we want on newlib.
#if defined(__clang__)
// Host clang (bindgen): hand off to the real glibc <features.h>.
#include_next <features.h>
#else
#include <sys/features.h>
#endif
