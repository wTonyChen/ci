// Nintendo Switch POSIX gap fillers for the Chromium/Starboard target build.
#include <stdlib.h>
#include <time.h>

extern "C" {

// perfetto's base/time.h calls timegm; newlib has no timegm.
time_t timegm(struct tm* tm) {
  return mktime(tm);
}

}  // extern "C"
