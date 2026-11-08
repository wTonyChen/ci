// Minimal M3/M4 probe: exercises Chromium's build output + newlib + libnx and
// prints the result on screen so a real console can prove it ran.
//
// The entry point (_start) comes from libnx.a; it initialises the runtime and
// calls main(), exactly like a regular devkitPro homebrew build.

#include <stdio.h>

#include <switch.h>

#include "third_party/zlib/zlib.h"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  consoleInit(NULL);

  const uLong crc = crc32(0, Z_NULL, 0);

  printf("nx_smoke -- Chromium //base toolchain probe\n");
  printf("\n");
  printf("  zlib crc32(empty) = %lu\n", static_cast<unsigned long>(crc));
  printf("  (expected 0)\n");
  printf("\n");
  printf("compiled with Chromium's GN/ninja + clang + devkitA64\n");
  printf("\n");
  printf("press + / - / A to exit\n");
  consoleUpdate(NULL);

  PadState pad;
  padInitializeDefault(&pad);
  while (appletMainLoop()) {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) &
        (HidNpadButton_Plus | HidNpadButton_Minus | HidNpadButton_A)) {
      break;
    }
    consoleUpdate(NULL);
    svcSleepThread(50000000ULL);  // 50 ms
  }

  consoleExit(NULL);
  return 0;
}
