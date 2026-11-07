// mmap/munmap/mprotect/madvise bring-up shim for Switch (no real VM available).
//
// Ported from the hardware-verified probe (probes/src/mmap/mmap_shim.c,
// "OK=10 FAIL=0" on FW 21.0.1). Keep the two copies in sync if either changes.
//
// DESIGN / LIMITS (documented honestly):
//  * devkitA64 has no mmap at all, and libnx exposes no general virtual-memory
//    allocator -- only virtmem address reservation + svcMapMemory (which MOVES
//    memory and on 2.0.0+ can only target the stack region). That is unusable as
//    a general mmap.
//  * So this shim implements mmap as an ARENA over memalign()ed chunks:
//      - anonymous mappings  -> bump allocation from 4KiB-aligned chunks
//      - munmap              -> marked free, exact-size reuse only (no coalescing)
//      - mprotect            -> recorded, NOT enforced (stays RW). Kept permissive
//                               so bring-up never faults; revisit before shipping.
//      - file-backed mapping -> read() the bytes into the arena
//      - MAP_FIXED over our own region -> accepted (reserve-then-commit pattern)
//      - madvise             -> no-op (no demand paging to give back)
//  * It is enough to get PartitionAlloc / V8 style allocators off the ground.
//    It is NOT true virtual memory: no demand paging, no guard pages, no
//    overcommit, and PROT_NONE is not enforced.
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

namespace {

constexpr size_t kNxPage = 0x1000u;
constexpr int kNxMaxRecords = 8192;
constexpr size_t kNxChunk = 32u * 1024u * 1024u;

constexpr size_t AlignUp(size_t x) {
  return (x + kNxPage - 1) & ~(static_cast<size_t>(kNxPage) - 1);
}

struct Rec {
  void* addr;
  size_t size;
  bool used;
  int prot;
};

Rec g_rec[kNxMaxRecords];
int g_nrec;
char* g_bump;
size_t g_left;
bool g_inited;

bool EnsureInit() {
  if (g_inited) {
    return true;
  }
  g_inited = true;
  g_bump = static_cast<char*>(aligned_alloc(kNxPage, kNxChunk));
  if (!g_bump) {
    g_left = 0;
    return false;
  }
  memset(g_bump, 0, kNxPage);
  g_left = kNxChunk;
  return true;
}

Rec* RecNew(void* addr, size_t size, bool used, int prot) {
  if (g_nrec >= kNxMaxRecords) {
    return nullptr;
  }
  Rec* r = &g_rec[g_nrec++];
  r->addr = addr;
  r->size = size;
  r->used = used;
  r->prot = prot;
  return r;
}

// Finds a record containing [addr, addr+len).
Rec* RecFind(void* addr, size_t len) {
  for (int i = 0; i < g_nrec; i++) {
    Rec* r = &g_rec[i];
    if (static_cast<char*>(r->addr) <= static_cast<char*>(addr) &&
        static_cast<char*>(addr) + len <= static_cast<char*>(r->addr) + r->size) {
      return r;
    }
  }
  return nullptr;
}

void* Reuse(size_t size) {
  for (int i = 0; i < g_nrec; i++) {
    Rec* r = &g_rec[i];
    if (!r->used && r->size == size) {
      r->used = true;
      return r->addr;
    }
  }
  return nullptr;
}

}  // namespace

extern "C" {

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  if (length == 0) {
    return MAP_FAILED;
  }
  const size_t size = AlignUp(length);

  // reserve-then-commit: MAP_FIXED onto something we already handed out
  if ((flags & MAP_FIXED) && addr) {
    Rec* r = RecFind(addr, size);
    if (r) {
      r->used = true;
      r->prot = prot;
      return r->addr;
    }
    errno = ENOMEM;  // we cannot map arbitrary addresses
    return MAP_FAILED;
  }

  void* p = Reuse(size);
  if (!p) {
    if (!EnsureInit()) {
      errno = ENOMEM;
      return MAP_FAILED;
    }
    if (size > g_left) {
      const size_t chunk = size > kNxChunk ? size : kNxChunk;
      char* nb = static_cast<char*>(aligned_alloc(kNxPage, chunk));
      if (!nb) {
        errno = ENOMEM;
        return MAP_FAILED;
      }
      g_bump = nb;
      g_left = chunk;
    }
    p = g_bump;
    g_bump += size;
    g_left -= size;
  }

  if (!RecNew(p, size, true, prot)) {
    errno = ENOMEM;
    return MAP_FAILED;
  }

  if (fd >= 0 && !(flags & MAP_ANONYMOUS)) {
    // newlib has no pread(); use lseek + read
    if (lseek(fd, offset, SEEK_SET) < 0) {
      errno = EIO;
      return MAP_FAILED;
    }
    if (read(fd, p, length) < 0) {
      errno = EIO;
      return MAP_FAILED;
    }
  }
  return p;
}

int munmap(void* addr, size_t length) {
  Rec* r = RecFind(addr, length ? length : 1);
  if (!r) {
    errno = EINVAL;
    return -1;
  }
  r->used = false;  // kept for exact-size reuse; memory is not returned
  return 0;
}

int mprotect(void* addr, size_t length, int prot) {
  Rec* r = RecFind(addr, length ? length : 1);
  if (!r) {
    errno = EINVAL;
    return -1;
  }
  r->prot = prot;  // recorded only; see the file comment above
  return 0;
}

int madvise(void* addr, size_t length, int advice) {
  (void)addr;
  (void)length;
  (void)advice;
  return 0;  // nothing to release: the arena never pages
}

}  // extern "C"
