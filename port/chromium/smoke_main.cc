// Minimal M3 link/run probe: exercises libbase.a + newlib + libnx through the
// Switch link script. Built by //:nx_smoke (see BUILD.gn).
//
// _start is provided here because the Chromium toolchain does not use the
// devkitA64 crt0 (which calls libnx's initialisation); the point of M3 is to
// prove the *link* works, not to boot.

#include <stdio.h>
#include <time.h>

#include <switch.h>

#include "base/time/time.h"

namespace {

void RunSmokeTest() {
  const int64_t ms = base::Time::Now().InMillisecondsSinceUnixEpoch();
  printf("nx_smoke: base::Time::Now() = %lld ms\n", static_cast<long long>(ms));
}

}  // namespace

extern "C" void _start(void) {
  RunSmokeTest();
  for (;;) {
    svcSleepThread(1000000000ULL);  // 1 s, keeps the ELF at a stable state
  }
}
