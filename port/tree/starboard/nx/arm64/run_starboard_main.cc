// Nintendo Switch (libnx) entry point for Starboard.
//
// The Linux version instantiates starboard::ApplicationX11 and calls glibc-only
// helpers (mallopt/crash-signal handlers/backtrace). Here we use the portable
// QueueApplication; windowing is provided by the nx platform layer instead.

#include <time.h>

#include "starboard/shared/starboard/link_receiver.h"
#include "starboard/shared/starboard/queue_application.h"

int SbRunStarboardMain(int argc, char** argv, SbEventHandleCallback callback) {
  tzset();
  starboard::QueueApplication application(callback);
  starboard::LinkReceiver receiver(&application);
  return application.Run(argc, argv);
}
