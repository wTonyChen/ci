// nx_big_probe —— 纯 libnx、零 Chromium，用来区分「1.1 MB 的 NRO 是不是加载器/配额问题」。
//
// 设计要点（和 STATUS §2.4 第 2 步的地基探针同构，另加一条尺寸变量）：
//   1) 每写一行日志就立刻 fsFileWrite 到 sdmc:/nxbig.log —— 不用 malloc、不用 stdio 缓冲，
//      后面就算卡死/崩掉，SD 上也能留下最后到达的那一行（面包屑）。
//   2) 打一行 loader 信息（envGetLoaderInfo）+ 上一个 NRO 的 load result，
//      顺便把「Tony 到底用哪个加载器」这件事留证。
//   3) 一个 912288 字节的 .data 数组，把 NRO 撑到和 nx_base_probe 同量级，
//      用来测「同样的文件体积，换成零 Chromium 的包还能不能加载」。
//      main 里对这个数组求校验和，能算完就证明 data 段整段被正确映射。
//   4) ctor(0) 写一个固定 marker，main 打出来 —— 证明构造器先于 main 跑到了。

#include <stdio.h>
#include <string.h>
#include <switch.h>

// .data 段、文件里实打实占 912288 字节（只有第一个字节非零，避免被折进 .bss）
#ifndef PAD_BYTES
#define PAD_BYTES 912288  // 默认：把 codebin 顶到 0x10c000，与失败包同尺寸
#endif
static volatile unsigned char g_pad[PAD_BYTES] = {0x5A};

static FsFileSystem g_sdfs;
static FsFile g_file;
static s64 g_off;
static int g_sd_ok;   // 0=没开  1=开好

static u32 g_ctor_marker;  // ctor(0) 写，main 读

__attribute__((constructor(0))) static void probe_ctor0(void) {
    g_ctor_marker = 0xC0FFEE01u;
}

// ---------------- 极简日志：一条一条写，绝不缓存 ----------------
static void log_line(const char* s) {
    printf("%s\n", s);
    consoleUpdate(NULL);
    if (!g_sd_ok) return;
    size_t n = strlen(s);
    if (fsFileWrite(&g_file, g_off, s, n, 0) == 0 &&
        fsFileWrite(&g_file, g_off + (s64)n, "\n", 1, 0) == 0)
        g_off += (s64)n + 1;
}

static void logf2(const char* k, u64 v) {
    char buf[160];
    snprintf(buf, sizeof buf, "%s=%#llx", k, (unsigned long long)v);
    log_line(buf);
}

static void logf_str(const char* k, const char* v) {
    char buf[320];
    snprintf(buf, sizeof buf, "%s=%s", k, v ? v : "(null)");
    log_line(buf);
}

static void sd_open(void) {
    char buf[160];
    Result rc;

    rc = fsInitialize();  // libnx 可能已经初始化过，失败不当致命
    snprintf(buf, sizeof buf, "P2 fsInitialize rc=%#x", (unsigned)rc);
    log_line(buf);

    rc = fsOpenSdCardFileSystem(&g_sdfs);
    snprintf(buf, sizeof buf, "P3 fsOpenSdCardFileSystem rc=%#x", (unsigned)rc);
    log_line(buf);
    if (R_FAILED(rc)) return;

    rc = fsFsCreateFile(&g_sdfs, "/nxbig.log", 0, 0);  // 已存在会失败，忽略
    snprintf(buf, sizeof buf, "P4 create rc=%#x (exist ok)", (unsigned)rc);
    log_line(buf);

    rc = fsFsOpenFile(&g_sdfs, "/nxbig.log", FsOpenMode_Write, &g_file);
    snprintf(buf, sizeof buf, "P5 open-write rc=%#x", (unsigned)rc);
    log_line(buf);
    if (R_FAILED(rc)) return;

    s64 sz = 0;
    if (R_SUCCEEDED(fsFileGetSize(&g_file, &sz))) g_off = sz;
    g_sd_ok = 1;
    logf2("P6 sd ready, start offset", (u64)g_off);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    consoleInit(NULL);
    log_line("P1 consoleInit ok  (probe alive)");

    sd_open();

    logf2("P7 ctor0 marker (expect 0xc0ffee01)", g_ctor_marker);
    logf_str("P8 loader info", envGetLoaderInfo());
    logf2("P9 last load result", (u64)envGetLastLoadResult());
    logf2("P10 envIsNso", envIsNso() ? 1 : 0);
    logf_str("P11 own process handle below", "");
    logf2("P12 own process handle", (u64)envGetOwnProcessHandle());
    logf2("P13 heap override present", envHasHeapOverride() ? 1 : 0);
    logf2("P14 heap override size", envGetHeapOverrideSize());

    // data 段整段校验：算得完 = data 段完整映射
    u64 sum = 0;
    for (u32 i = 0; i < PAD_BYTES; i++) sum += g_pad[i];
    logf2("P15 pad sum", sum);
    logf2("P16 pad first/last", ((u64)g_pad[0] << 8) | g_pad[PAD_BYTES - 1]);

    log_line("P17 sleeping 5s (console stays up)");
    consoleUpdate(NULL);
    svcSleepThread(5000000000ULL);

    log_line("P18 done, exiting");
    consoleUpdate(NULL);
    if (g_sd_ok) fsFileClose(&g_file);
    consoleExit(NULL);
    return 0;
}
