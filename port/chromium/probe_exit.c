/* Bisect probe A: smallest possible homebrew. No console, no services -- just
 * leave through the libnx crt0 exit path. If this crashes on FW 23, the problem
 * is the toolchain/libnx startup itself, not anything we do in main(). */
#include <switch.h>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  svcSleepThread(2000000000ULL);  /* 2 s so a human sees the app "ran" */
  return 0;
}
