// M2 bisect step 0: link //base but call *nothing* from it. If this still
// crashes, the problem is in base's static initialisers or in the relocations
// of the larger image, not in anything main() does.
#include <stdio.h>

#include <switch.h>

extern "C" int nx_console_is_ready;
extern "C" void nx_log_str(const char*);
extern "C" int nx_log_flush(void);

__attribute__((constructor(0))) static void early_breadcrumb() {
  // NOTE: consoleInit() allocates through PA here, and PA may still be in its
  // own initialisation -> calling it this early hangs the process. Keep the
  // constructor allocation-free; the console comes up in main() instead.
  nx_console_is_ready = 0;
  nx_log_str("ctor(0) reached\n");
  nx_log_flush();
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  nx_log_str("main reached\n");
  nx_log_flush();
  consoleInit(NULL);
  nx_console_is_ready = 1;
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
