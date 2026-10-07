// mmap/munmap/mprotect bring-up shim for Switch (no real VM available).
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
//  * It is enough to get PartitionAlloc / V8 style allocators off the ground.
//    It is NOT true virtual memory: no demand paging, no guard pages, no
//    overcommit, and PROT_NONE is not enforced.
#include "mman.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define NX_PAGE   0x1000u
#define NX_NREC   8192
#define NX_CHUNK  (32u * 1024u * 1024u)

#define ALIGN_UP(x) (((x) + NX_PAGE - 1) & ~((size_t)NX_PAGE - 1))

typedef struct { void *addr; size_t size; int used; int prot; } rec_t;

static rec_t  g_rec[NX_NREC];
static int    g_nrec;
static char  *g_bump;          // current chunk bump pointer
static size_t g_left;          // bytes left in current chunk
static int    g_inited;

static int ensure_init(void)
{
    if (g_inited) return 0;
    g_inited = 1;
    g_bump = (char *)aligned_alloc(NX_PAGE, NX_CHUNK);
    if (!g_bump) { g_bump = NULL; g_left = 0; return -1; }
    memset(g_bump, 0, NX_PAGE);
    g_left = NX_CHUNK;
    return 0;
}

static rec_t *rec_new(void *addr, size_t size, int used, int prot)
{
    if (g_nrec >= NX_NREC) return NULL;
    rec_t *r = &g_rec[g_nrec++];
    r->addr = addr; r->size = size; r->used = used; r->prot = prot;
    return r;
}

// find a record containing [addr, addr+len)
static rec_t *rec_find(void *addr, size_t len)
{
    for (int i = 0; i < g_nrec; i++) {
        rec_t *r = &g_rec[i];
        if ((char *)r->addr <= (char *)addr &&
            (char *)addr + len <= (char *)r->addr + r->size) return r;
    }
    return NULL;
}

// exact-size free reuse
static void *reuse(size_t size)
{
    for (int i = 0; i < g_nrec; i++) {
        rec_t *r = &g_rec[i];
        if (!r->used && r->size == size) { r->used = 1; return r->addr; }
    }
    return NULL;
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (length == 0) return MAP_FAILED;
    size_t size = ALIGN_UP(length);

    // reserve-then-commit: MAP_FIXED onto something we already handed out
    if ((flags & MAP_FIXED) && addr) {
        rec_t *r = rec_find(addr, size);
        if (r) { r->used = 1; r->prot = prot; return r->addr; }
        errno = ENOMEM;           // we cannot map arbitrary addresses
        return MAP_FAILED;
    }

    void *p = reuse(size);
    if (!p) {
        if (ensure_init() != 0) { errno = ENOMEM; return MAP_FAILED; }
        if (size > g_left) {
            size_t chunk = size > NX_CHUNK ? size : NX_CHUNK;
            char *nb = (char *)aligned_alloc(NX_PAGE, chunk);
            if (!nb) { errno = ENOMEM; return MAP_FAILED; }
            g_bump = nb; g_left = chunk;
        }
        p = g_bump;
        g_bump += size;
        g_left -= size;
    }

    if (!rec_new(p, size, 1, prot)) { errno = ENOMEM; return MAP_FAILED; }

    if (fd >= 0 && !(flags & MAP_ANONYMOUS)) {
        // newlib has no pread(); use lseek+read
        if (lseek(fd, offset, SEEK_SET) < 0) { errno = EIO; return MAP_FAILED; }
        ssize_t got = read(fd, p, length);
        if (got < 0) { errno = EIO; return MAP_FAILED; }
    }
    return p;
}

int munmap(void *addr, size_t length)
{
    rec_t *r = rec_find(addr, length ? length : 1);
    if (!r) { errno = EINVAL; return -1; }
    r->used = 0;                  // kept for exact-size reuse; memory is not returned
    return 0;
}

int mprotect(void *addr, size_t length, int prot)
{
    rec_t *r = rec_find(addr, length ? length : 1);
    if (!r) { errno = EINVAL; return -1; }
    r->prot = prot;               // recorded only; see header comment
    return 0;
}
