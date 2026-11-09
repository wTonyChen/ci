// Minimal Chromium-build-system hello: same shape as the devkitPro example that
// is known to run on hardware (consoleInit + printf + consoleExit), but compiled
// and linked through our GN/ninja setup. If this crashes while the devkitPro
// hello-world.nro runs, the difference is in *our* toolchain flags, not in the
// Chromium code.
#include <stdio.h>

#include <switch.h>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  consoleInit(NULL);
  printf("nx_hello from Chromium's build system\n");
  printf("press + / - / A to exit\n");
  consoleUpdate(NULL);

  // padGetButtons* only reports anything once hid has been initialised; the
  // libnx crt0 does not do it for us.
  hidInitialize();
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
  return 0;
}
