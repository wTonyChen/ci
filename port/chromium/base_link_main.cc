// M2 bisect step 0: link //base but call *nothing* from it. If this still
// crashes, the problem is in base's static initialisers or in the relocations
// of the larger image, not in anything main() does.
#include <stdio.h>

#include <switch.h>

__attribute__((constructor(101))) static void early_breadcrumb() {
  consoleInit(NULL);
  printf("nx_base_link: pre-main init reached\n");
  printf("(sleeping 3s so this is readable)\n");
  consoleUpdate(NULL);
  svcSleepThread(3000000000ULL);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  consoleInit(NULL);
  printf("nx_base_link -- //base linked, no base:: calls\n");
  printf("if you can read this, constructors + relocations are fine\n");
  printf("press + / - / A to exit\n");
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
  return 0;
}
