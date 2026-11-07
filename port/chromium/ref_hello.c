/* Control sample: the SAME hello-world behaviour, but built with devkitPro's
 * own GCC toolchain + switch.specs (no Chromium build system involved).
 *
 * Its only purpose is diagnosis: if this NRO runs on hardware but the
 * Chromium-built one does not, the difference is our toolchain/link setup;
 * if both fail, the problem is the environment (Sphaira/AMS/Application mode).
 */
#include <stdio.h>
#include <switch.h>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  consoleInit(NULL);
  printf("ref_hello -- devkitPro GCC control sample\n\n");
  printf("if you can read this, the official toolchain works here.\n\n");
  printf("press + to exit\n");
  consoleUpdate(NULL);

  PadState pad;
  padInitializeDefault(&pad);
  while (appletMainLoop()) {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) & HidNpadButton_Plus) {
      break;
    }
    consoleUpdate(NULL);
    svcSleepThread(50000000ULL);
  }
  consoleExit(NULL);
  return 0;
}
