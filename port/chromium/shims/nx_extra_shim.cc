// Extra bring-up shims: the handful of POSIX/C-runtime entry points that
// newlib + libsysbase + libnx do not provide for the Switch target.
//
// NOTE: PartitionsAlloc (TagMemoryRange*/GetAccessFlags) and Chromium's
// StackTrace helper used to live here too; they are only needed when linking
// //base, which the M3 smoke target deliberately avoids (see BUILD.gn).

#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {

// crti.o/crtbegin.o are not part of our link line (we link -nostdlib), but
// libnx's startup still references these hooks.
void _init(void) {}
void _fini(void) {}

// newlib has memalign() but not POSIX posix_memalign().
int posix_memalign(void** memptr, size_t alignment, size_t size) {
  if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0) {
    return EINVAL;
  }
  void* p = memalign(alignment, size);
  if (!p) {
    return ENOMEM;
  }
  *memptr = p;
  return 0;
}

// Rust core (and some C code) calls bcmp; newlib only has memcmp.
int bcmp(const void* s1, const void* s2, size_t n) {
  return memcmp(s1, s2, n);
}

ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
  const off_t saved = lseek(fd, 0, SEEK_CUR);
  if (saved < 0) {
    return -1;
  }
  if (lseek(fd, offset, SEEK_SET) < 0) {
    return -1;
  }
  const ssize_t n = read(fd, buf, count);
  lseek(fd, saved, SEEK_SET);
  return n;
}

ssize_t writev(int fd, const void* iov, int iovcnt) {
  (void)fd;
  (void)iov;
  (void)iovcnt;
  errno = ENOSYS;
  return -1;
}

long sysconf(int name) {
  switch (name) {
    case 30:  // _SC_PAGESIZE (Linux numbering)
      return 4096;
    case 39:  // _SC_NPROCESSORS_ONLN
    case 40:  // _SC_NPROCESSORS_CONF
      return 3;  // Horizon exposes 3 application cores
    default:
      errno = EINVAL;
      return -1;
  }
}

int getpagesize(void) {
  return 4096;
}

unsigned long getauxval(unsigned long type) {
  (void)type;
  errno = ENOENT;
  return 0;
}

int pthread_getname_np(pthread_t thread, char* name, size_t len) {
  (void)thread;
  if (len) {
    name[0] = '\0';
  }
  return 0;
}

}  // extern "C"
