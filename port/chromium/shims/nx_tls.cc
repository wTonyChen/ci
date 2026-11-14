// nx_tls.cc —— 静态 TLS 的落地实现。
//
// 思路：linker script（libnx switch.ld）已经把 .tdata/.tbss 的大小留在
// __tls_start/__tls_end 之间，但没有任何东西把 TP 设过去。我们自己分配一块
// 每线程的区，把 .tdata 的初值映进去、TPIDR_EL0 指过去，thread_local 就正常了。
//
// 两个必须踩准的点（都是踩过的坑）：
//   1) AArch64 静态 TLS 的 TP 比 .tdata 起点低 16 字节（TP_OFFSET），
//      所以 .tdata 的影像要落在 TP + 16；clang 下发的固定偏移正好差的这 16。
//   2) 光清零不够：`constinit thread_local` 对象的 vptr 存在 .tdata 影像里
//      （靠 RELATIVE 重定位写成最终地址），不映影像的话每线程的 vptr 都是 0，
//      第一次调用虚函数就崩在 0x10（L4 base::Thread 就是这么崩的）。
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <pthread.h>
#include <switch.h>
#include <sys/mman.h>

#include "nx_tls.h"

extern "C" {
extern const char __tls_start[];
extern const char __tls_end[];
// .tdata 的初值影像（linker script 给的）。缺了也不至于链接失败。
extern const char __tdata_lma[] __attribute__((weak));
extern const char __tdata_lma_end[] __attribute__((weak));
}

namespace {

constexpr size_t kPage = 0x1000;
// AArch64 静态 TLS：TP 指向块起点，第一个 TLS 变量在 TP + 0x10。
constexpr size_t kTlsBias = 0x10;
constexpr int kMaxBlocks = 128;

void* g_blocks[kMaxBlocks];
int g_block_count = 0;

inline uintptr_t ReadTp() {
  uintptr_t tp;
  __asm__ volatile("mrs %0, tpidr_el0" : "=r"(tp));
  return tp;
}

inline void WriteTp(uintptr_t tp) {
  __asm__ volatile("msr tpidr_el0, %0" ::"r"(tp));
}

bool IsOurBlock(uintptr_t tp) {
  if (tp == 0)
    return false;
  for (int i = 0; i < g_block_count; ++i) {
    if (reinterpret_cast<uintptr_t>(g_blocks[i]) == tp)
      return true;
  }
  return false;
}

}  // namespace

extern "C" void nx_tls_setup_current_thread(void) {
  const uintptr_t old_tp = ReadTp();
  if (IsOurBlock(old_tp))
    return;

  size_t tls_size = static_cast<size_t>(__tls_end - __tls_start);
  if (tls_size == 0 || tls_size > (1u << 20))
    tls_size = kPage;
  const size_t total = tls_size + kTlsBias;
  const size_t size = ((total + kPage - 1) / kPage) * kPage;

  void* block = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (block == MAP_FAILED)
    return;
  memset(block, 0, size);

  // .tdata 影像：constinit thread_local 的 vptr/初值都在这里。
  if (__tdata_lma != nullptr && __tdata_lma_end != nullptr) {
    size_t tdata_size = static_cast<size_t>(__tdata_lma_end - __tdata_lma);
    if (tdata_size > tls_size)
      tdata_size = tls_size;
    memcpy(static_cast<char*>(block) + kTlsBias, __tdata_lma, tdata_size);
  }

  WriteTp(reinterpret_cast<uintptr_t>(block));
  if (ReadTp() != reinterpret_cast<uintptr_t>(block)) {
    // 写不进去就还原，让上层看到原来的错误而不是一个更奇怪的错误。
    WriteTp(old_tp);
    return;
  }

  for (int i = 0; i < kMaxBlocks; ++i) {
    if (g_blocks[i] == nullptr) {
      g_blocks[i] = block;
      if (i + 1 > g_block_count)
        g_block_count = i + 1;
      break;
    }
  }
}

extern "C" uintptr_t nx_tls_read_word(size_t off) {
  const uintptr_t tp = ReadTp();
  if (tp == 0)
    return 0;
  return *reinterpret_cast<const uintptr_t*>(tp + off);
}

// --- 线程：每个新线程也要有自己的 TLS 块 ---------------------------------

namespace {
struct StartArgs {
  void* (*fn)(void*);
  void* arg;
};

void* Trampoline(void* p) {
  StartArgs* a = static_cast<StartArgs*>(p);
  void* (*fn)(void*) = a->fn;
  void* arg = a->arg;
  free(a);
  nx_tls_setup_current_thread();
  return fn(arg);
}
}  // namespace

extern "C" int __real_pthread_create(pthread_t*, const pthread_attr_t*,
                                     void* (*)(void*), void*);

// Debug aid for the L5 investigation: when a probe sets nx_thread_trace = 1 we
// log every pthread_create() attempt (and its result) to sdmc:/nxlog.txt, so a
// crash inside base::ThreadPool still tells us how many threads it got to.
extern "C" int nx_thread_trace;
extern "C" void nx_log_str(const char*);
extern "C" int nx_log_flush(void);

static int g_threads_created;

static int AppendUint(char* out, unsigned value) {
  char tmp[10];
  int n = 0;
  do {
    tmp[n++] = static_cast<char>('0' + value % 10);
    value /= 10;
  } while (value != 0);
  for (int i = 0; i < n; ++i) {
    out[i] = tmp[n - 1 - i];
  }
  return n;
}



extern "C" int nx_threads_created(void) {
  return g_threads_created;
}

extern "C" int __wrap_pthread_create(pthread_t* t, const pthread_attr_t* attr,
                                     void* (*fn)(void*), void* arg) {
  StartArgs* a = static_cast<StartArgs*>(malloc(sizeof(StartArgs)));
  if (a == nullptr)
    return -1;  // ENOMEM 的量级，调用方会当失败处理
  a->fn = fn;
  a->arg = arg;
  const int rc = __real_pthread_create(t, attr, Trampoline, a);
  if (rc == 0) {
    ++g_threads_created;
  }
  if (nx_thread_trace) {
    // Hand-rolled formatting: this runs before/around allocator setup and must
    // not depend on printf.
    char line[80];
    int n = 0;
    const char* pre = "thread create #";
    while (*pre) line[n++] = *pre++;
    n += AppendUint(line + n, static_cast<unsigned>(g_threads_created));
    const char* mid = " rc=";
    while (*mid) line[n++] = *mid++;
    n += AppendUint(line + n, static_cast<unsigned>(rc < 0 ? 0 : rc));
    line[n++] = '\n';
    line[n] = 0;
    nx_log_str(line);
    nx_log_flush();
  }
  return rc;
}
