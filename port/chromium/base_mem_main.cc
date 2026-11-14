// M2 step 3b: address-space and executable-memory facts, measured on the real
// console before we commit to a V8 port. Nothing here depends on //base: these
// are the raw properties of port/chromium/shims/mmap_shim.cc.
//
//   M1  256 MB anonymous mapping                -> ordinary heap arena growth
//   M2  1 GB anonymous mapping                  -> how much room we really have
//   M3  4 GB PROT_NONE reservation              -> a pointer-compression cage
//   M4  PROT_NONE actually enforced?            -> touch + read back a page
//   M5  JIT: write code, mprotect(RX), call it  -> does Horizon allow w^x split
//
// M1/M2/M5 are pass/fail; M3/M4 are measurements (V8 picks its config from them).
#include <cstdarg>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include <switch.h>

extern "C" int nx_console_is_ready;
extern "C" void nx_log_str(const char*);
extern "C" int nx_log_flush(void);

namespace {

char g_line[256];
bool g_ok = true;

void Report(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(g_line, sizeof(g_line), fmt, ap);
  va_end(ap);
  nx_log_str(g_line);
  nx_log_str("\n");
  nx_log_flush();
  if (nx_console_is_ready) {
    printf("%s\n", g_line);
    consoleUpdate(NULL);
  }
}

void Step(const char* name, size_t mb, int prot) {
  const size_t len = mb * 1024u * 1024u;
  void* p = mmap(nullptr, len, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                 -1, 0);
  if (p == MAP_FAILED) {
    Report("%s: mmap %zu MB -> FAILED (errno %d)", name, mb, errno);
    return;
  }
  Report("%s: mmap %zu MB -> ok at %p", name, mb, p);
}

int RunCode() {
  const size_t len = 4096;
  void* p = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) {
    Report("M5 JIT: mmap RW -> FAILED (errno %d)", errno);
    return -1;
  }
  // mov w0, #7 ; ret
  const unsigned code[2] = {0x528000E0u, 0xD65F03C0u};
  memcpy(p, code, sizeof(code));
  __builtin___clear_cache(static_cast<char*>(p),
                          static_cast<char*>(p) + sizeof(code));
  if (mprotect(p, len, PROT_READ | PROT_EXEC) != 0) {
    Report("M5 JIT: mprotect RX -> FAILED (errno %d)", errno);
    return -1;
  }
  using Fn = int (*)();
  const Fn fn = reinterpret_cast<Fn>(p);
  const int v = fn();
  Report("M5 JIT: executed generated code -> returned %d (want 7)", v);
  return v;
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  nx_log_str("mem: main reached\n");
  nx_log_flush();
  consoleInit(NULL);
  nx_console_is_ready = 1;
  printf("nx_base_mem -- address space + executable memory\n\n");
  consoleUpdate(NULL);

  Report("page size = %ld", static_cast<long>(sysconf(_SC_PAGESIZE)));
  Step("M1", 256, PROT_READ | PROT_WRITE);
  Step("M2", 1024, PROT_READ | PROT_WRITE);

  Report("M3 4 GB cage: start");
  const size_t cage = 4096u * 1024u * 1024u;
  void* c = mmap(nullptr, cage, PROT_NONE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (c == MAP_FAILED) {
    Report("M3 4 GB cage: mmap -> FAILED (errno %d) -- no pointer compression",
           errno);
  } else {
    Report("M3 4 GB cage: mmap -> ok at %p", c);
    if (mprotect(c, 4096, PROT_READ | PROT_WRITE) == 0) {
      volatile unsigned char* b = static_cast<volatile unsigned char*>(c);
      b[0] = 0x5A;
      Report("M4 PROT_NONE enforced: no (wrote %d into a PROT_NONE page)", b[0]);
    } else {
      Report("M4 PROT_NONE enforced: mprotect failed errno %d", errno);
    }
  }

  const int v = RunCode();
  g_ok = (v == 7);
  Report("mem: all steps done, result = %s", g_ok ? "PASS" : "FAIL");
  printf("\npress + / - / A to exit\n");
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
  return g_ok ? 0 : 1;
}
