// probe_posix -- spike #3 JIT, #4 memory, #5 POSIX gaps
// ASCII ONLY (libnx console font is ASCII). Press + to exit.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <switch.h>

#if defined(__has_include)
#  if __has_include(<sys/mman.h>)
#    include <sys/mman.h>
#    define HAVE_MMAN_H 1
#  endif
#endif

static int n_ok = 0, n_missing = 0, n_fail = 0;

extern void   *mmap(void *, size_t, int, int, int, off_t)      __attribute__((weak));
extern int     mprotect(void *, size_t, int)                    __attribute__((weak));
extern int     munmap(void *, size_t)                           __attribute__((weak));
extern int     pipe(int[2])                                     __attribute__((weak));
extern int     pipe2(int[2], int)                               __attribute__((weak));
extern int     socketpair(int, int, int, int[2])                __attribute__((weak));
extern int     eventfd(unsigned int, int)                       __attribute__((weak));
extern int     epoll_create1(int)                               __attribute__((weak));
extern int     epoll_ctl(int, int, int, void *)                 __attribute__((weak));
extern int     epoll_wait(int, void *, int, int)                __attribute__((weak));
extern int     inotify_init1(int)                               __attribute__((weak));
extern int     inotify_add_watch(int, const char *, unsigned)   __attribute__((weak));
extern ssize_t getrandom(void *, size_t, unsigned)              __attribute__((weak));
extern unsigned long getauxval(unsigned long)                   __attribute__((weak));
extern int     statx(int, const char *, int, unsigned, void *)  __attribute__((weak));
extern int     pthread_detach(pthread_t)                        __attribute__((weak));
extern int     pthread_getattr_np(pthread_t, pthread_attr_t *)  __attribute__((weak));
extern int     sem_init(void *, int, unsigned)                  __attribute__((weak));
extern int     sem_wait(void *)                                 __attribute__((weak));
extern int     sem_post(void *)                                 __attribute__((weak));
extern int     sem_destroy(void *)                              __attribute__((weak));
extern long    sysconf(int)                                     __attribute__((weak));

#define SYM(lbl, fn) do { printf("  %-18s ", lbl);                       \
    if (!(fn)) { printf("MISSING\n"); n_missing++; }                     \
    else       { printf("present\n"); n_ok++; } consoleUpdate(NULL); } while (0)

#define LN(lbl, ...) do { printf("  %-18s ", lbl); printf(__VA_ARGS__);  \
    printf("\n"); consoleUpdate(NULL); } while (0)

static void sep(const char *t) { printf("\n-- %s --\n", t); consoleUpdate(NULL); }

static void probe_env(void)
{
    sep("A. environment");
    AppletType at = appletGetAppletType();
    const char *mode = (at == AppletType_Application) ? "Application(full mem)"
                     : (at == AppletType_SystemApplication) ? "SystemApplication"
                     : (at == AppletType_LibraryApplet) ? "LibraryApplet(LOW MEM)" : "other";
    LN("mode", "%s raw=%d", mode, (int)at);
    if (at != AppletType_Application) LN("WARNING", "not Application mode, limits apply");
    u32 fw = hosversionGet();
    LN("firmware", "%d.%d.%d", (fw >> 16) & 0xFF, (fw >> 8) & 0xFF, fw & 0xFF);
    LN("gcc", "%s", __VERSION__);
    u64 total = 0, used = 0;
    if (R_SUCCEEDED(svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)))
        LN("TotalMemorySize", "%llu MB", (unsigned long long)(total / 1024 / 1024));
    if (R_SUCCEEDED(svcGetInfo(&used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0)))
        LN("UsedMemorySize", "%llu MB", (unsigned long long)(used / 1024 / 1024));
}

static void probe_symbols(void)
{
    sep("B. POSIX symbols (Cobalt/Starboard 17+ needs these)");
    SYM("mmap", mmap);                     SYM("mprotect", mprotect);
    SYM("munmap", munmap);                 SYM("pipe", pipe);
    SYM("pipe2", pipe2);                   SYM("socketpair", socketpair);
    SYM("eventfd", eventfd);               SYM("epoll_create1", epoll_create1);
    SYM("epoll_ctl", epoll_ctl);           SYM("epoll_wait", epoll_wait);
    SYM("inotify_init1", inotify_init1);   SYM("inotify_add_watch", inotify_add_watch);
    SYM("getrandom", getrandom);           SYM("getauxval", getauxval);
    SYM("statx", statx);                   SYM("pthread_detach", pthread_detach);
    SYM("pthread_getattr_np", pthread_getattr_np);
    SYM("sem_init", sem_init);             SYM("sem_wait", sem_wait);
    SYM("sem_post", sem_post);             SYM("sem_destroy", sem_destroy);
    SYM("sysconf", sysconf);
}

static void probe_runtime(void)
{
    sep("C. runtime behaviour");
#ifdef HAVE_MMAN_H
    if (mmap) {
        errno = 0;
        void *p = mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p != MAP_FAILED) {
            memset(p, 0xAB, 65536);
            LN("mmap", "[ OK ] 65536 bytes @ %p", p);
            if (mprotect) LN("mprotect", "rc=%d errno=%d", mprotect(p, 65536, PROT_READ), errno);
            if (munmap) munmap(p, 65536);
        } else { LN("mmap", "[FAIL] errno=%d %s", errno, strerror(errno)); n_fail++; }
    }
#else
    LN("mmap", "[SKIP] no <sys/mman.h> at all");
#endif
    if (pipe) {
        int fds[2] = {-1, -1};
        errno = 0;
        if (pipe(fds) == 0) {
            char c = 'x', r = 0;
            write(fds[1], &c, 1); read(fds[0], &r, 1);
            LN("pipe", "[ OK ] rw ok data=%c", r ? r : '?');
            close(fds[0]); close(fds[1]);
        } else { LN("pipe", "[FAIL] errno=%d %s", errno, strerror(errno)); n_fail++; }
    }
    if (socketpair) {
        Result bsd = socketInitializeDefault();
        if (R_SUCCEEDED(bsd)) {
            int sv[2] = {-1, -1};
            errno = 0;
            if (socketpair(AF_INET, SOCK_STREAM, 0, sv) == 0) {
                LN("socketpair", "[ OK ] %d,%d", sv[0], sv[1]); close(sv[0]); close(sv[1]);
            } else { LN("socketpair", "[FAIL] errno=%d %s", errno, strerror(errno)); n_fail++; }
            socketExit();
        } else LN("socketpair", "[SKIP] bsd init 0x%08x", bsd);
    }
    if (eventfd) {
        errno = 0;
        int fd = eventfd(0, 0);
        if (fd >= 0) { LN("eventfd", "[ OK ] fd=%d", fd); close(fd); }
        else { LN("eventfd", "[FAIL] errno=%d %s", errno, strerror(errno)); n_fail++; }
    }
    if (epoll_create1) {
        errno = 0;
        int fd = epoll_create1(0);
        if (fd >= 0) { LN("epoll_create1", "[ OK ] fd=%d", fd); close(fd); }
        else { LN("epoll_create1", "[FAIL] errno=%d %s", errno, strerror(errno)); n_fail++; }
    }
    if (getrandom) {
        unsigned char b[16] = {0};
        ssize_t n = getrandom(b, sizeof(b), 0);
        LN("getrandom", "n=%d (want 16)", (int)n);
    }
    if (getauxval) LN("getauxval", "AT_HWCAP=0x%lx", getauxval(16));
    struct timespec ts;
    LN("clock MONOTONIC", "rc=%d", clock_gettime(CLOCK_MONOTONIC, &ts));
    LN("clock REALTIME", "rc=%d", clock_gettime(CLOCK_REALTIME, &ts));
#ifdef CLOCK_MONOTONIC_RAW
    LN("clock MONO_RAW", "rc=%d", clock_gettime(CLOCK_MONOTONIC_RAW, &ts));
#endif
    if (sysconf) LN("sysconf", "PAGESIZE=%ld NPROC=%ld", sysconf(_SC_PAGESIZE), sysconf(_SC_NPROCESSORS_ONLN));
    else LN("sysconf", "[MISSING] not implemented");
}

static void *thread_fn(void *arg) { (void)arg; return (void *)0x1234; }

static void probe_threads(void)
{
    sep("D. threads");
#ifdef PTHREAD_STACK_MIN
    LN("PTHREAD_STACK_MIN", "%ld bytes", (long)PTHREAD_STACK_MIN);
#else
    LN("PTHREAD_STACK_MIN", "undefined");
#endif
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) == 0) {
        size_t defsz = 0;
        pthread_attr_getstacksize(&attr, &defsz);
        LN("default stack", "%zu bytes (%.0f KB)", defsz, defsz / 1024.0);
        pthread_attr_destroy(&attr);
    }
    pthread_attr_t a2;
    pthread_t th;
    if (pthread_attr_init(&a2) == 0) {
        pthread_attr_setstacksize(&a2, 1024 * 1024);
        errno = 0;
        int rc = pthread_create(&th, &a2, thread_fn, NULL);
        if (rc == 0) { void *ret = NULL; pthread_join(th, &ret); LN("1MiB stack thread", "[ OK ] ret=%p", ret); }
        else { LN("1MiB stack thread", "[FAIL] rc=%d errno=%d", rc, errno); n_fail++; }
        pthread_attr_destroy(&a2);
    }
}

static void probe_memory(void)
{
    sep("E. large malloc");
    static const size_t sizes[] = { 128u<<20, 256u<<20, 512u<<20, 1024u<<20, 1536u<<20, 2048u<<20 };
    for (unsigned i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        size_t sz = sizes[i];
        printf("  malloc %4zu MB ... ", sz >> 20); consoleUpdate(NULL);
        void *p = malloc(sz);
        if (!p) { printf("[FAIL] NULL\n"); n_fail++; consoleUpdate(NULL); continue; }
        if (sz <= (512u << 20)) { memset(p, 0x5A, sz); printf("[ OK ] fully written\n"); }
        else { memset(p, 0x5A, 4096); memset((char *)p + sz - 4096, 0x5A, 4096); printf("[ OK ] first/last page ok\n"); }
        n_ok++; free(p); consoleUpdate(NULL);
    }
}

typedef int (*fn_void_t)(void);

static void probe_jit(void)
{
    sep("F. JIT (libnx jitCreate, no V8 needed)");
    Jit jit;
    Result rc = jitCreate(&jit, 0x1000);
    if (R_FAILED(rc)) {
        LN("jitCreate", "[FAIL] 0x%08x -> LibnxError_JitUnavailable?", rc);
        printf("  >> V8 would have to run jitless\n"); n_fail++; return;
    }
    LN("jitCreate", "[ OK ] rw=%p rx=%p", jitGetRwAddr(&jit), (void *)jitGetRxAddr(&jit));
    rc = jitTransitionToWritable(&jit);
    if (R_FAILED(rc)) { LN("to writable", "[FAIL] 0x%08x", rc); n_fail++; jitClose(&jit); return; }
    u32 *code = (u32 *)jitGetRwAddr(&jit);
    code[0] = 0x52800540u;   // mov w0, #42
    code[1] = 0xD65F03C0u;   // ret
    rc = jitTransitionToExecutable(&jit);
    if (R_FAILED(rc)) { LN("to executable", "[FAIL] 0x%08x", rc); n_fail++; jitClose(&jit); return; }
    {
        char *rx = (char *)(void *)jitGetRxAddr(&jit);
        __builtin___clear_cache(rx, rx + 8);
        fn_void_t fn = (fn_void_t)(void *)rx;
        int v = fn();
        if (v == 42) { LN("exec generated", "[ OK ] returned %d -- JIT REALLY WORKS", v); n_ok++; }
        else { LN("exec generated", "[FAIL] returned %d (want 42)", v); n_fail++; }
    }
    jitClose(&jit);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    consoleInit(NULL);
    printf("=== probe_posix : Switch capability report ===\n");
    consoleUpdate(NULL);
    probe_env();
    probe_symbols();
    probe_runtime();
    probe_threads();
    probe_memory();
    probe_jit();
    sep("summary");
    printf("  ok=%d  missing=%d  fail=%d\n", n_ok, n_missing, n_fail);
    printf("\n  Press + to exit\n");
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
