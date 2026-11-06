// Nintendo Switch: newlib ships no <sys/mman.h> at all, and Horizon exposes no
// general-purpose virtual memory API (only virtmem address reservation +
// svcMapMemory, which MOVES memory). So the port supplies both the header and
// an arena-based implementation -- see shims/mmap_shim.cc for the honest list
// of limitations.
#ifndef FAKE_SYS_MMAN_H_
#define FAKE_SYS_MMAN_H_

#include <stddef.h>
#include <sys/types.h>

#ifndef PROT_NONE
#define PROT_NONE 0x0
#endif
#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef PROT_EXEC
#define PROT_EXEC 0x4
#endif

#ifndef MAP_SHARED
#define MAP_SHARED 0x01
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0x02
#endif
#ifndef MAP_FIXED
#define MAP_FIXED 0x10
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif
#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0x4000
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

#ifndef MADV_NORMAL
#define MADV_NORMAL 0
#endif
#ifndef MADV_RANDOM
#define MADV_RANDOM 1
#endif
#ifndef MADV_SEQUENTIAL
#define MADV_SEQUENTIAL 2
#endif
#ifndef MADV_WILLNEED
#define MADV_WILLNEED 3
#endif
#ifndef MADV_DONTNEED
#define MADV_DONTNEED 4
#endif

#ifndef MS_ASYNC
#define MS_ASYNC 1
#endif
#ifndef MS_INVALIDATE
#define MS_INVALIDATE 2
#endif
#ifndef MS_SYNC
#define MS_SYNC 4
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void* addr, size_t length);
int mprotect(void* addr, size_t length, int prot);
int madvise(void* addr, size_t length, int advice);

// Horizon memory is ordinary RAM: msync has nothing to flush.
static inline int msync(void* addr, size_t length, int flags) {
  (void)addr;
  (void)length;
  (void)flags;
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_MMAN_H_
