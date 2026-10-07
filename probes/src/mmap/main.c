// probe_mmap -- validates the mmap bring-up shim on real hardware.
// ASCII only (libnx console font is ASCII). Press + to exit.
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "mman.h"
#include <switch.h>

static int n_ok = 0, n_fail = 0;
#define OK(...)   do { printf("  [ OK ] " __VA_ARGS__); printf("\n"); n_ok++;   consoleUpdate(NULL); } while (0)
#define FAIL(...) do { printf("  [FAIL] " __VA_ARGS__); printf("\n"); n_fail++; consoleUpdate(NULL); } while (0)
#define SKIP(...) do { printf("  [skip] " __VA_ARGS__); printf("\n");           consoleUpdate(NULL); } while (0)

static void test_anon(const char *label, size_t sz)
{
    errno = 0;
    unsigned char *p = (unsigned char *)mmap(NULL, sz, PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { FAIL("%-14s %8zu B: mmap errno=%d", label, sz, errno); return; }
    if (((uintptr_t)p & 0xFFF) != 0) { FAIL("%-14s not page aligned (%p)", label, p); return; }
    memset(p, 0xA5, sz);                       // touch every byte
    if (p[0] != 0xA5 || p[sz - 1] != 0xA5) { FAIL("%-14s write/read mismatch", label); return; }
    OK("%-14s %8zu B @ %p  (written+verified)", label, sz, p);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    consoleInit(NULL);
    printf("=== probe_mmap (arena shim) ===\n\n");

    test_anon("tiny",       16);
    test_anon("page",       4096);
    test_anon("64K",        64 * 1024);
    test_anon("1M",         1u << 20);
    test_anon("16M",        16u << 20);
    test_anon("64M",        64u << 20);

    // many small allocations (allocator pattern)
    {
        void *v[512]; int ok = 0;
        for (int i = 0; i < 512; i++) {
            v[i] = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (v[i] != MAP_FAILED) { memset(v[i], i & 0xFF, 4096); ok++; }
        }
        if (ok == 512) OK("512 x 4K allocs all usable"); else FAIL("only %d/512 allocs", ok);
    }

    // munmap then exact-size reuse
    {
        void *a = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (a == MAP_FAILED) FAIL("munmap test: first mmap");
        else if (munmap(a, 8192) != 0) FAIL("munmap returned error");
        else {
            void *b = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (b == a) OK("munmap -> same-size reuse returned same address");
            else if (b != MAP_FAILED) OK("munmap ok (reuse elsewhere: %p vs %p)", a, b);
            else FAIL("re-mmap failed");
        }
    }

    // mprotect transitions (permissive by design)
    {
        unsigned char *p = (unsigned char *)mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) FAIL("mprotect test: mmap");
        else {
            p[0] = 1;
            int r1 = mprotect(p, 4096, PROT_READ);
            p[1] = 2;                       // stays writable on purpose
            int r2 = mprotect(p, 4096, PROT_READ | PROT_WRITE);
            p[2] = 3;
            if (r1 == 0 && r2 == 0 && p[0] == 1 && p[1] == 2 && p[2] == 3)
                OK("mprotect RO->RW transitions accepted");
            else FAIL("mprotect r1=%d r2=%d", r1, r2);
        }
    }

    // file-backed mapping (data read into the arena)
    {
        const char *path = "sdmc:/probe_mmap_data.bin";
        FILE *f = fopen(path, "wb");
        if (!f) SKIP("file-backed test (cannot write %s)", path);
        else {
            char buf[1024];
            for (int i = 0; i < 1024; i++) buf[i] = (char)(i & 0x7F);
            fwrite(buf, 1, sizeof(buf), f); fclose(f);
            int fd = open(path, O_RDONLY);
            if (fd < 0) FAIL("open(%s)", path);
            else {
                char *m = (char *)mmap(NULL, 1024, PROT_READ, MAP_PRIVATE, fd, 0);
                if (m == MAP_FAILED) FAIL("file-backed mmap errno=%d", errno);
                else if (m[0] == 0 && m[100] == (char)(100 & 0x7F)) OK("file-backed mmap content OK");
                else FAIL("file-backed content wrong (%d,%d)", m[0], m[100]);
                close(fd);
            }
        }
    }

    printf("\n========================\n");
    printf("OK=%d FAIL=%d\n", n_ok, n_fail);
    printf("%s\n", n_fail == 0 ? ">> mmap shim behaves as designed"
                               : ">> some tests failed, see above");
    printf("\nPress + to exit\n");
    consoleUpdate(NULL);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus) break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return 0;
}
