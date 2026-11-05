// Nintendo Switch: newlib has <sys/features.h> but no top-level <features.h>
// (that is glibc's). partition_alloc's build_config.h includes <features.h> on
// the Linux path, so forward it.
#ifndef FAKE_FEATURES_H_
#define FAKE_FEATURES_H_

#include <sys/features.h>

#endif  // FAKE_FEATURES_H_
