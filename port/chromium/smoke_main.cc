// Minimal M3 link/run probe: exercises libbase.a + newlib + libnx through the
// Switch link script. Built by //:nx_smoke (see BUILD.gn).
//
// The entry point (_start) comes from libnx.a and calls main() after libnx
// initialisation, exactly like a regular devkitPro homebrew build.

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

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  RunSmokeTest();
  return 0;
}
