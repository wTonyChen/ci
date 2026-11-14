// M2 step 3: the event loop. This is the first probe that needs the virtual fd
// layer (port/chromium/shims/nxfd_shim.cc): base::MessagePumpDefault is built
// around an eventfd that it polls and writes to, and Horizon has neither.
//
// Steps, cheapest first, each preceded by a breadcrumb (sdmc:/nxlog.txt + screen):
//   L1 SingleThreadTaskExecutor  -> eventfd + poll come up at all
//   L2 immediate PostTask        -> task queue draining works
//   L3 delayed PostTask (10 ms)  -> timer path + poll timeout/rewake
//   L4 base::Thread start/stop   -> a second pump on a second OS thread
//   L5 thread pool, 16 tasks     -> many workers, one shared queue
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>

#include <switch.h>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/time/time.h"

extern "C" int nx_console_is_ready;
extern "C" void nx_log_str(const char*);
extern "C" int nx_log_flush(void);

namespace {

char g_line[256];
bool g_all_ok = true;
// Must outlive every step below: destroying it tears down the message pump that
// L2-L5 keep using (each RunLoop/Thread borrows the current thread's runner).
std::unique_ptr<base::SingleThreadTaskExecutor> g_exec;

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

void Fail(const char* step, const char* why) {
  Report("%s: FAIL %s", step, why);
  g_all_ok = false;
}

// ---- L1/L2: executor + immediate task -------------------------------------
void StepImmediate() {
  Report("L1 executor: start");
  g_exec = std::make_unique<base::SingleThreadTaskExecutor>();
  Report("L1 executor: ok (eventfd + pump constructed)");

  Report("L2 immediate task: start");
  int ran = 0;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce([](int* out) { *out = 7; }, &ran));
  base::RunLoop().RunUntilIdle();
  if (ran == 7) {
    Report("L2 immediate task: ok (task ran, value %d)", ran);
  } else {
    Fail("L2 immediate task", "task did not run");
  }
}

// ---- L3: delayed task (the timer + re-wake path) --------------------------
void StepDelayed() {
  Report("L3 delayed task: start");
  const base::TimeTicks t0 = base::TimeTicks::Now();
  base::TimeTicks fired_at;
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](base::RunLoop* loop, base::TimeTicks* fired) {
            *fired = base::TimeTicks::Now();
            loop->Quit();
          },
          &run_loop, &fired_at),
      base::Milliseconds(10));
  run_loop.Run();
  const base::TimeDelta waited = fired_at - t0;
  if (waited.InMilliseconds() >= 8) {
    Report("L3 delayed task: ok (10 ms timer fired after %lld ms)",
           static_cast<long long>(waited.InMilliseconds()));
  } else {
    Report("L3 delayed task: FAIL fired too early (%lld ms)",
           static_cast<long long>(waited.InMilliseconds()));
    g_all_ok = false;
  }
}

// ---- L4: a second thread with its own pump --------------------------------
void StepThread() {
  Report("L4 base::Thread: start");
  base::Thread worker("nx_probe");
  if (!worker.Start()) {
    Fail("L4 base::Thread", "Start() returned false");
    return;
  }
  std::atomic<int> seen{0};
  base::RunLoop loop;
  worker.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](std::atomic<int>* v, base::RunLoop* l) {
            *v = static_cast<int>(
                base::PlatformThread::CurrentId() & 0xffff);
            l->Quit();
          },
          &seen, &loop));
  loop.Run();
  worker.Stop();
  if (seen.load() != 0) {
    Report("L4 base::Thread: ok (worker tid low bits 0x%x, stopped cleanly)",
           seen.load());
  } else {
    Fail("L4 base::Thread", "task on worker never ran");
  }
}

// ---- L5: thread pool ------------------------------------------------------
void StepPool() {
  Report("L5 thread pool: start");
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("nx_probe");
  std::atomic<int> done{0};
  base::RunLoop loop;
  for (int i = 0; i < 16; ++i) {
    base::ThreadPool::PostTask(
        FROM_HERE, base::BindOnce(
                       [](std::atomic<int>* d, base::RunLoop* l) {
                         if (++(*d) == 16) {
                           l->Quit();
                         }
                       },
                       &done, &loop));
  }
  loop.Run();
  base::ThreadPoolInstance::Get()->Shutdown();
  Report("L5 thread pool: ok (%d/16 tasks done)", done.load());
  if (done.load() != 16) {
    g_all_ok = false;
  }
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  nx_log_str("loop: main reached\n");
  nx_log_flush();
  consoleInit(NULL);
  nx_console_is_ready = 1;
  printf("nx_base_loop -- //base event loop on Switch\n\n");
  consoleUpdate(NULL);

  StepImmediate();
  StepDelayed();
  StepThread();
  StepPool();

  Report("loop: all steps done, result = %s", g_all_ok ? "PASS" : "FAIL");
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
