// Minimal Starboard application for the Nintendo Switch port.
//
// Its only purpose right now is to reach the link stage: linking it against the
// platform pulls in every libc/POSIX symbol Starboard actually uses, which is
// exactly the list the port has to provide (mmap, pipes, epoll, ...).
#include "starboard/event.h"
#include "starboard/log.h"
#include "starboard/system.h"

void SbEventHandle(const SbEvent* event) {
  switch (event->type) {
    case kSbEventTypeStart:
      SbLogRawFormatF("chromium-nx: SbEventHandle(kSbEventTypeStart)\n");
      SbSystemRequestStop(0);
      break;
    default:
      SbLogRawFormatF("chromium-nx: event type %d\n", event->type);
      break;
  }
}
