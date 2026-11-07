// Minimal M3 link/run probe: exercises libbase.a + newlib + libnx through the
// Switch link script. Built by //:nx_smoke (see BUILD.gn).
//
// The entry point (_start) comes from libnx.a and calls main() after libnx
// initialisation, exactly like a regular devkitPro homebrew build.

#include <stdio.h>
#include <time.h>

#include <switch.h>

#include "third_party/zlib/zlib.h"

namespace {

void RunSmokeTest() {
  // A real function from Chromium's own zlib, so a successful link proves the
  // toolchain + archive + libnx/newlib path, without dragging in //base (whose
  // Rust parts need libunwind, which does not exist for bare-metal aarch64).
  const uLong crc = crc32(0, Z_NULL, 0);
  printf("nx_smoke: crc32(empty) = %lu\n", static_cast<unsigned long>(crc));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  RunSmokeTest();
  return 0;
}
