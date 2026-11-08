/* Bisect probe B: + consoleInit/consoleExit (gfx + applet services). */
#include <stdio.h>
#include <switch.h>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  consoleInit(NULL);
  consoleUpdate(NULL);
  svcSleepThread(2000000000ULL);
  consoleExit(NULL);
  return 0;
}
