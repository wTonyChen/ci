// Minimal QueueApplication for Nintendo Switch.
//
// TODO(chromium-nx): drive this from libnx instead of the stub behaviour:
//   * applet state changes (appletMainLoop / appletGetOperationMode)
//   * HID input (padUpdate / hidGet*)
//   * window events, and a real wakeup primitive (libnx Event or a pipe)
// For now it mirrors //starboard/stub/application_stub.* so the platform builds
// and the generic queue/timed-event machinery can run.
#ifndef STARBOARD_NX_ARM64_APPLICATION_NX_H_
#define STARBOARD_NX_ARM64_APPLICATION_NX_H_

#include "starboard/shared/starboard/application.h"
#include "starboard/shared/starboard/queue_application.h"

namespace starboard {
namespace nx {

class ApplicationNx : public QueueApplication {
 public:
  explicit ApplicationNx(SbEventHandleCallback sb_event_handle_callback)
      : QueueApplication(sb_event_handle_callback) {}
  ~ApplicationNx() override {}

  static ApplicationNx* Get() {
    return static_cast<ApplicationNx*>(Application::Get());
  }

 protected:
  void Initialize() override {}
  void Teardown() override {}
  bool MayHaveSystemEvents() override { return false; }
  Event* PollNextSystemEvent() override { return NULL; }
  Event* WaitForSystemEventWithTimeout(int64_t /*time*/) override {
    return NULL;
  }
  void WakeSystemEventWait() override {}
};

}  // namespace nx
}  // namespace starboard

#endif  // STARBOARD_NX_ARM64_APPLICATION_NX_H_
