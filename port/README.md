# Switch 移植层（libnx-posix）设计说明

目标：为 Cobalt/Starboard-18 + Chromium `base` + V8 在 **devkitA64 + libnx** 上提供它们假设存在的 POSIX 面。
所有模块都以「**一个模块 + 一个上机探针**」的方式交付：探针在真机上跑出 `OK=n FAIL=m`，不靠推断。

## 分层结构

```
L4  epoll          epoll_create1 / epoll_ctl / epoll_wait   ← 建立在 L1 包装后的 poll 上
L3  对象           pipe2 / eventfd / socketpair / timerfd   ← 全部注册进 L1
L2  虚拟 fd 层      nxfd_alloc/lookup/dispatch             ← 高位 fd + 分派
L1  系统调用包装    read/write/close/poll/fcntl/ioctl
L0  mmap 家族      mmap / munmap / mprotect                ← 已实现并真机验证（OK=10 FAIL=0）
```

### 为什么必须有虚拟 fd 层

libnx 只对 `bsd:u`（socket）的 fd 提供 `poll` 支持；而 `pipe`/`eventfd`/`socketpair` 在 Switch 上**根本不存在**（`socketpair` 返回 errno 88，`AF_UNIX` 返回 106）。
Chromium `base` 的消息循环依赖「对 pipe/eventfd 做 poll」这一模式，所以必须自己造一套 fd：

- 虚拟 fd 从高位起分配（如 `0x40000000 + n`），与 libnx 的真实 fd 不冲突；
- 包装 `read`/`write`/`close`/`poll`/`fcntl`/`ioctl`：**命中虚拟 fd 就自己处理，否则透传给 libnx**；
- `poll` 包装是关键：一次 `poll` 里可能同时有真实 socket fd 与虚拟 fd，需要「真实 fd 走 `poll()`，虚拟 fd 走各自的可读/可写判定，然后合并超时」。

### 各对象的最小语义

| 对象 | 设计 | 备注 |
|---|---|---|
| `pipe2` | 环形缓冲 + 两个虚拟 fd（读端/写端） | 容量取 64KiB；读端可读条件 = 缓冲非空或写端全关 |
| `eventfd` | 8 字节计数器 + 虚拟 fd | 支持 `EFD_NONBLOCK`；`EFD_CLOEXEC` 空实现；read 清零、write 累加 |
| `socketpair` | 进程内双向队列 + 两个虚拟 fd | 支持 `send`/`recv`/`read`/`write`/`shutdown`；`AF_UNIX` 仅此一种用法 |
| `epoll` | 基于 L1 的 `poll` 实现，**level-triggered** | `EPOLLET` 用 level-triggered 模拟（多几次唤醒，语义安全）；`EPOLLONESHOT` 需真实支持（触发后自动摘除） |

## 纯符号类（无 fd 语义，直接填）

- `pread` / `pwrite`：newlib 没有 → 用 `lseek` + `read`/`write` + 全局互斥实现
- `getrandom`：`randomGet` / `csrng` 之上的薄封装
- `getauxval`：按需返回（Chromium 主要用来问 `AT_PAGESZ` / `AT_HWCAP`）
- `sysconf`：只需支持 Chromium 实际读取的几个：`_SC_PAGESIZE`、`_SC_NPROCESSORS_ONLN`、`_SC_PHYS_PAGES`、`_SC_OPEN_MAX`
- `statx`：退化为 `stat` 填结构
- `pthread_getattr_np`：返回默认栈范围（实测默认栈查询返回 0，必须自己给）

## 已知硬限制（配置层面必须绕开）

1. **无 `PROT_NONE` / 无 guard page**：PartitionAlloc 守护页、V8 W^X 需要另想办法（JIT 走 libnx `jitCreate` 的 RW/RX 别名）。
2. **无 overcommit / 无需求分页**：稀疏大预留会真吃内存；**V8 指针压缩 cage 的 4GB 虚拟预留必须换策略**（先读 devkitPro `switch-v8 15.0.243-9` 的构建配置作参照）。
3. **无 `AF_INET6`**（errno 106）：Starboard-17+ 假设 IPv6 可用，需在 Cobalt 侧关掉或用 `AF_INET` 打通。

## 交付顺序

| 阶段 | 内容 | 探针 |
|---|---|---|
| ✅ 已完成 | `mmap` / `munmap` / `mprotect` | `probe_mmap`（OK=10 FAIL=0） |
| 1 | nxfd 虚拟 fd 层 + `pipe2` + `eventfd` + `poll` 包装 | `probe_poll` |
| 2 | `epoll_*`（含 ONESHOT / ET 模拟） | `probe_epoll` |
| 3 | `socketpair` + `send`/`recv`/`shutdown` | `probe_socketpair` |
| 4 | 纯符号若干（`pread`/`pwrite`/`getrandom`/`sysconf`/`getauxval`/`statx`/`pthread_getattr_np`） | `probe_posix2` |
| 5 | Starboard 平台文件 `starboard/nx/arm64/`（另一条线，见评估报告 §4/§10.8） | — |

源码落位约定：内核无关的 shim 放 `port/libnx-posix/`，探针放 `setup/probes/src/<模块>/`；等阶段 1 完成后再把 shim 从探针目录抽出来成独立静态库。
