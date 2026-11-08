/* Bisect probe C: + printf through the console (newlib stdio path). */
#include <stdio.h>
#include <switch.h>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  consoleInit(NULL);
  printf("probe_printf: hello\n");
  consoleUpdate(NULL);
  svcSleepThread(2000000000ULL);
  consoleExit(NULL);
  return 0;
}
