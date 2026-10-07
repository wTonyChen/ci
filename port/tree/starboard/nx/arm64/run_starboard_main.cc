// Nintendo Switch (libnx) entry point for Starboard.
//
// The Linux version instantiates starboard::ApplicationX11 and calls glibc-only
// helpers (mallopt/crash-signal handlers/backtrace). Here we use the portable
// QueueApplication; windowing is provided by the nx platform layer instead.

#include <time.h>

#include "starboard/nx/arm64/application_nx.h"
#include "starboard/shared/starboard/queue_application.h"

int SbRunStarboardMain(int argc, char** argv, SbEventHandleCallback callback) {
  tzset();
  starboard::nx::ApplicationNx application(callback);
  return application.Run(argc, argv);
}
