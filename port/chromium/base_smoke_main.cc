// M2 step 1: prove that //base actually LINKS into a Switch binary and that a
// couple of its code paths run. Keep it to facilities that do not need threads,
// timers or the filesystem yet -- we add those one by one once the basic link
// is green.
#include <stdio.h>

#include <string>

#include <switch.h>

#include "base/strings/stringprintf.h"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  consoleInit(NULL);
  printf("nx_base_smoke -- Chromium //base on Switch\n\n");
  consoleUpdate(NULL);

  const std::string s = base::StringPrintf("%d:%.2f", 42, 1.5);
  printf("  base::StringPrintf -> \"%s\"\n", s.c_str());
  printf("  std::string works, size=%zu\n", s.size());
  printf("\npress + / - / A to exit\n");
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
    svcSleepThread(50000000ULL);
  }

  consoleExit(NULL);
  return 0;
}
