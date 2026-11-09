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
  Result hid_rc = hidInitialize();
  printf("hidInitialize rc=0x%08x\n", hid_rc);
  // hid reports nothing until the supported Npad style set / id list has been
  // configured; padUpdate() skips any controller whose style_set reads 0.
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  printf("style set No1=0x%x Handheld=0x%x\n",
         (unsigned)hidGetNpadStyleSet(HidNpadIdType_No1),
         (unsigned)hidGetNpadStyleSet(HidNpadIdType_Handheld));
  PadState pad;
  padInitializeDefault(&pad);

  int frame = 0;
  while (appletMainLoop()) {
    padUpdate(&pad);
    const u64 down = padGetButtonsDown(&pad);
    const u64 cur = padGetButtons(&pad);
    if (frame % 30 == 0) {
      printf("frame %d buttons=0x%llx down=0x%llx\n", frame,
             (unsigned long long)cur, (unsigned long long)down);
      consoleUpdate(NULL);
    }
    if (down & (HidNpadButton_Plus | HidNpadButton_Minus | HidNpadButton_A)) {
      printf("exit key seen: down=0x%llx\n", (unsigned long long)down);
      consoleUpdate(NULL);
      break;
    }
    frame++;
    svcSleepThread(50000000ULL);
  }
  printf("loop ended (appletMainLoop false) at frame %d\n", frame);
  consoleUpdate(NULL);
  svcSleepThread(2000000000ULL);

  consoleExit(NULL);
  return 0;
}
