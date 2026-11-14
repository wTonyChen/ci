// M2 step 2: the runtime facilities a browser needs right after //base links:
// clocks, sleeping, file IO, callbacks, and OS threads.
//
// Every step logs a breadcrumb *before* it runs, through the zero-malloc SD log
// channel (sdmc:/nxlog.txt) and, once the console is up, to the screen. If the
// process dies inside a step, the last line in nxlog.txt names it, and the
// crash report's `module + offset` can be symbolized against the same artifact.
//
// Order matters: cheapest and least risky first (time -> sleep -> file ->
// callback), threads last -- they are the first thing that touches the Switch
// TLS story (clang emits `mrs tpidr_el0`; devkitPro's -mtp=soft is rejected by
// clang, see BUILD.gn).
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include <switch.h>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/stringprintf.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"

extern "C" int nx_console_is_ready;
extern "C" void nx_log_str(const char*);
extern "C" int nx_log_flush(void);

namespace {

char g_line[256];
bool g_all_ok = true;

// One printf-style call -> one SD log line + one console line.
void Report(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(g_line, sizeof(g_line), fmt, ap);
  va_end(ap);
  nx_log_str(g_line);
  nx_log_str("\n");
  nx_log_flush();
  if (nx_console_is_ready) {
    printf("%s\n", g_line);
    consoleUpdate(NULL);
  }
}

// ---- S1: clocks -----------------------------------------------------------
void StepClocks() {
  Report("S1 clocks: start");
  const base::TimeTicks t0 = base::TimeTicks::Now();
  const base::Time w0 = base::Time::Now();
  // Busy-wait ~5 ms so a frozen clock is visible rather than hidden by rounding.
  while (base::TimeTicks::Now() - t0 < base::Milliseconds(5)) {
  }
  const base::TimeDelta d = base::TimeTicks::Now() - t0;
  Report("S1 clocks: ok, monotonic busy-wait = %lld us, wall clock null = %d",
         static_cast<long long>(d.InMicroseconds()),
         w0.is_null() ? 1 : 0);
  if (d.InMicroseconds() <= 0) {
    g_all_ok = false;
  }
}

// ---- S2: sleeping ---------------------------------------------------------
void StepSleep() {
  Report("S2 sleep: start");
  const base::TimeTicks t0 = base::TimeTicks::Now();
  base::PlatformThread::Sleep(base::Milliseconds(20));
  const base::TimeDelta d = base::TimeTicks::Now() - t0;
  Report("S2 sleep: ok, 20 ms requested, slept %lld us",
         static_cast<long long>(d.InMicroseconds()));
  if (d.InMilliseconds() < 10) {
    g_all_ok = false;
  }
}

// ---- S3: file IO through //base (newlib devoptab -> sdmc:) ------------------
void StepFile() {
  Report("S3 file: start");
  base::File file(base::FilePath(FILE_PATH_LITERAL("sdmc:/nxrt.txt")),
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_READ |
                      base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    Report("S3 file: FAILED to open sdmc:/nxrt.txt: %s",
           base::File::ErrorToString(file.error_details()).c_str());
    g_all_ok = false;
    return;
  }
  const std::string payload =
      base::StringPrintf("base::File works, %d\n", 42);
  // Offsets are explicit: WriteAtCurrentPos/ReadAtCurrentPos are private.
  const int written = file.Write(0, payload.data(),
                                 static_cast<int>(payload.size()));
  file.Flush();
  char buf[64] = {0};
  const int got = file.Read(0, buf, sizeof(buf) - 1);
  Report("S3 file: ok, wrote %d bytes, read back %d bytes: \"%s\"", written, got,
         buf);
  if (written != static_cast<int>(payload.size()) || got != written) {
    g_all_ok = false;
  }
}

// ---- S4: callbacks (no OS dependency, but every later layer uses them) ----
void StepCallbacks() {
  Report("S4 callbacks: start");
  // Binding 21 leaves a nullary callback; the RunType must match exactly.
  base::OnceCallback<int()> once =
      base::BindOnce([](int v) { return v * 2; }, 21);
  base::RepeatingCallback<std::string(const std::string&)> rep =
      base::BindRepeating([](const std::string& s) { return s + "!"; });
  const int v = std::move(once).Run();
  const std::string s = rep.Run("once");
  Report("S4 callbacks: ok, OnceCallback -> %d, RepeatingCallback -> \"%s\"", v,
         s.c_str());
  if (v != 42) {
    g_all_ok = false;
  }
}

// ---- S5: OS threads -------------------------------------------------------
class CountingDelegate : public base::PlatformThread::Delegate {
 public:
  void ThreadMain() override {
    Report("S5 thread: worker running (thread id %lu)",
           static_cast<unsigned long>(base::PlatformThread::CurrentId()));
    base::PlatformThread::Sleep(base::Milliseconds(20));
    ran_ = true;
  }
  bool ran_ = false;
};

void StepThread() {
  Report("S5 thread: start");
  CountingDelegate delegate;
  base::PlatformThreadHandle handle;
  const bool created = base::PlatformThread::Create(0, &delegate, &handle);
  if (!created) {
    Report("S5 thread: FAILED to create thread");
    g_all_ok = false;
    return;
  }
  base::PlatformThread::Join(handle);
  Report("S5 thread: ok, joined, worker ran = %d", delegate.ran_ ? 1 : 0);
  if (!delegate.ran_) {
    g_all_ok = false;
  }
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  nx_log_str("rt: main reached\n");
  nx_log_flush();
  consoleInit(NULL);
  nx_console_is_ready = 1;
  printf("nx_base_rt -- //base runtime facilities on Switch\n\n");
  consoleUpdate(NULL);

  StepClocks();
  StepSleep();
  StepFile();
  StepCallbacks();
  StepThread();

  Report("rt: all steps done, result = %s", g_all_ok ? "PASS" : "FAIL");
  printf("\npress + / - / A to exit\n");
  consoleUpdate(NULL);

  hidInitialize();
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  PadState pad;
  padInitializeDefault(&pad);
  while (appletMainLoop()) {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) &
        (HidNpadButton_Plus | HidNpadButton_Minus | HidNpadButton_A)) {
      break;
    }
    consoleUpdate(NULL);
    svcSleepThread(50000000ULL);
  }
  consoleExit(NULL);
  return g_all_ok ? 0 : 1;
}
