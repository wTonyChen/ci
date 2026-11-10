// M2 step 1: prove that //base actually LINKS into a Switch binary and that a
// couple of its code paths run. Keep it to facilities that do not need threads,
// timers or the filesystem yet -- we add those one by one once the basic link
// is green.
#include <stdio.h>

#include <string>

#include <switch.h>

extern "C" int nx_console_is_ready;

#include "base/strings/stringprintf.h"

// Breadcrumb: //base has global constructors that may run before main(). If the
// process dies inside them the screen would stay black and we would not know how
// far it got. A constructor with priority 101 runs before the ordinary ones and
// can show that libnx init + first console init already worked.
__attribute__((constructor(0))) static void early_breadcrumb() {
  // NOTE: consoleInit() allocates through PA here, and PA may still be in its
  // own initialisation -> calling it this early hangs the process. Keep the
  // constructor allocation-free; the console comes up in main() instead.
  nx_console_is_ready = 0;
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  consoleInit(NULL);
  nx_console_is_ready = 1;
  printf("nx_base_smoke -- Chromium //base on Switch\n\n");
  consoleUpdate(NULL);

  const std::string s = base::StringPrintf("%d:%.2f", 42, 1.5);
  printf("  base::StringPrintf -> \"%s\"\n", s.c_str());
  printf("  std::string works, size=%zu\n", s.size());
  printf("\npress + / - / A to exit\n");
  consoleUpdate(NULL);

  // padGetButtons* only reports anything once hid has been initialised; the
  // libnx crt0 does not do it for us.
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
