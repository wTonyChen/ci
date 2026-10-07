# Switch port layer (libnx-posix)

Goal: provide the POSIX surface that Cobalt/Starboard-18, Chromium `base` and V8 assume exists,
on top of **devkitA64 + libnx**. Every module ships as "one module + one on-device probe";
the probe prints `OK=n FAIL=m` on real hardware, so nothing is inferred.

## Layers

```
L4  epoll          epoll_create1 / epoll_ctl / epoll_wait   <- built on the wrapped poll
L3  objects        pipe2 / eventfd / socketpair / timerfd   <- registered in L2
L2  virtual fd     nxfd_alloc / lookup / dispatch           <- high fd numbers + dispatch
L1  syscall wrap   read / write / close / poll / fcntl / ioctl
L0  mmap family    mmap / munmap / mprotect                 <- done, validated (OK=10 FAIL=0)
```

### Why a virtual fd layer is mandatory

libnx only supports `poll()` on `bsd:u` (socket) fds. `pipe`/`eventfd`/`socketpair` do not exist on
Switch at all (`socketpair` returns errno 88, `AF_UNIX` returns 106). Chromium `base`'s message loop
assumes "poll on a pipe/eventfd", so the fds must be created by us:

- virtual fds are allocated from a high base (e.g. `0x40000000 + n`) so they never collide with libnx fds;
- `read`/`write`/`close`/`poll`/`fcntl`/`ioctl` are wrapped: virtual fds are handled locally, everything
  else is forwarded to libnx;
- the `poll` wrapper is the critical piece: one `poll` call may mix real socket fds and virtual fds, so
  real fds go to `poll()` while virtual fds report readiness from their own state, and the timeouts merge.

### Minimal semantics per object

| Object | Design | Notes |
|---|---|---|
| `pipe2` | ring buffer + two virtual fds | 64 KiB capacity; read end is ready when the buffer is non-empty or all write ends are closed |
| `eventfd` | 8-byte counter + virtual fd | `EFD_NONBLOCK` supported; `EFD_CLOEXEC` is a no-op; read resets, write adds |
| `socketpair` | in-process duplex queue + two virtual fds | supports `send`/`recv`/`read`/`write`/`shutdown` |
| `epoll` | implemented over the L1 `poll`, **level-triggered** | `EPOLLET` is emulated as level-triggered (extra wakeups only, semantics stay safe); `EPOLLONESHOT` must be real |

## Plain symbols (no fd semantics)

- `pread` / `pwrite`: missing in newlib -> `lseek` + `read`/`write` behind a global mutex
- `getrandom`: thin wrapper over `randomGet` / `csrng`
- `getauxval`: answer only what Chromium asks for (`AT_PAGESZ`, `AT_HWCAP`)
- `sysconf`: `_SC_PAGESIZE`, `_SC_NPROCESSORS_ONLN`, `_SC_PHYS_PAGES`, `_SC_OPEN_MAX`
- `statx`: degrade to `stat`
- `pthread_getattr_np`: report a default stack range (the default stack query returns 0 on hardware)

## Hard limits to work around at configuration level

1. **No `PROT_NONE` / no guard pages**: PartitionAlloc guard pages and V8 W^X need another mechanism
   (JIT uses libnx `jitCreate` RW/RX aliases).
2. **No overcommit / no demand paging**: sparse reservations consume real memory; the 4 GB virtual
   reservation V8 pointer compression needs must be replaced (see devkitPro `switch-v8 15.0.243-9`).
3. **No `AF_INET6`** (errno 106): Starboard-17+ assumes IPv6, so Cobalt must be configured for IPv4.

## Delivery order

| Stage | Content | Probe |
|---|---|---|
| done | `mmap` / `munmap` / `mprotect` | `probe_mmap` (OK=10 FAIL=0) |
| 1 | nxfd virtual fd layer + `pipe2` + `eventfd` + `poll` wrapper | `probe_poll` |
| 2 | `epoll_*` (ONESHOT, ET emulation) | `probe_epoll` |
| 3 | `socketpair` + `send`/`recv`/`shutdown` | `probe_socketpair` |
| 4 | plain symbols (`pread`/`pwrite`/`getrandom`/`sysconf`/`getauxval`/`statx`/`pthread_getattr_np`) | `probe_posix2` |
| 5 | Starboard platform files `starboard/nx/arm64/` (separate track) | - |
