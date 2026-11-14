// nx_tls.h —— 给 clang 编译的 Chromium 代码提供 AArch64 静态 TLS。
//
// Horizon 上 TPIDR_EL0 是「EL0 Read/Write Software Thread ID Register」：
// 内核不碰它，libnx 也不碰它（`nm libnx.a` 里没有一个对象引用 tpidr_el0），
// 所以它一直是 0。而 clang 把 thread_local 编译成 `mrs tpidr_el0` + 固定偏移，
// 于是第一次访问 TLS 变量就是「读 0x58」→ 0x0 附近取指 → Data Abort。
#pragma once

// 给当前线程准备一块零初始化的 TLS 区并把 TPIDR_EL0 指过去。
// 幂等：已经指到我们的块上就立刻返回。失败时保持原样（不制造更糟的状态）。
extern "C" void nx_tls_setup_current_thread(void);
