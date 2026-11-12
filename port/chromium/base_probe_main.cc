// Diagnostic: prove whether control ever reaches the first constructor when
// //base is linked. Does nothing else; reports through a user break (the AMS
// fatal screen shows the registers), because the console may not be usable.
#include <stdio.h>

#include <switch.h>

extern "C" void nx_log_str(const char*);
extern "C" int nx_log_flush(void);

// Runs even before the init_array constructors: .preinit_array is executed by
// __libc_init_array() before any constructor. If this does not run either, the
// hang is earlier still (crt0 / libnx init / dynamic relocations).
static void probe_preinit() __attribute__((section(".preinit_array"), used));
static void probe_preinit() {
  nx_log_str("probe preinit reached\n");
  nx_log_flush();
  svcBreak(2u, 0xAAAABBBBCCCCDDDDULL, 0x5A5A0002ULL);
}

__attribute__((constructor(0))) static void probe_ctor() {
  nx_log_str("probe ctor(0) reached\n");
  nx_log_flush();
  // X0 = BreakReason_User, X1 = magic marker, X2 = stage id.
  svcBreak(2u, 0x1111222233334444ULL, 0x5A5A0001ULL);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  // Should never be reached: the constructor above always breaks first.
  consoleInit(NULL);
  printf("probe main reached (constructor did NOT break)\n");
  consoleUpdate(NULL);
  svcSleepThread(2000000000ULL);
  consoleExit(NULL);
  return 0;
}
