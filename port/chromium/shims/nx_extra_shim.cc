// Extra bring-up shims needed by Chromium's //base: PartitionAlloc internals and
// a few POSIX calls newlib/devkitA64 do not provide.
//
// NOTE: the PartitionAlloc functions are declared here with the *exact* names and
// namespaces Chromium uses, so the mangled symbols match those in libbase.a.
// They are deliberately minimal (no MTE tagging, no thread isolation).

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// PartitionAlloc: TagMemoryRange* / RemaskPointer are ifunc-resolved on Linux
// with MTE; clang cannot emit ifunc for a bare-metal target, so they are stubs.
// ---------------------------------------------------------------------------
namespace partition_alloc::internal {

void* TagMemoryRangeRandomlyInternal(void* ptr, size_t size, uint64_t mask) {
  (void)size;
  (void)mask;
  return ptr;
}

void* TagMemoryRangeIncrementInternal(void* ptr, size_t size) {
  (void)size;
  return ptr;
}

void* RemaskPointerInternal(void* ptr) {
  return ptr;
}

}  // namespace partition_alloc::internal

// ---------------------------------------------------------------------------
// PartitionAlloc: GetAccessFlags(PageAccessibilityConfiguration).
// Re-declared with the same qualified names so the mangled symbol matches.
// ---------------------------------------------------------------------------
namespace partition_alloc {

struct PageAccessibilityConfiguration {
  enum Permissions {
    kInaccessible,
    kInaccessibleWillJitLater,
    kRead,
    kReadWrite,
    kReadWriteTagged,
    kReadExecuteProtected,
    kReadExecute,
    kReadWriteExecuteProtected,
    kReadWriteExecute,
  };
  Permissions permissions;
};

namespace internal {

int GetAccessFlags(PageAccessibilityConfiguration accessibility) {
  switch (accessibility.permissions) {
    case PageAccessibilityConfiguration::kInaccessible:
      return PROT_NONE;
    case PageAccessibilityConfiguration::kRead:
      return PROT_READ;
    case PageAccessibilityConfiguration::kReadWrite:
    case PageAccessibilityConfiguration::kReadWriteTagged:
      return PROT_READ | PROT_WRITE;
    case PageAccessibilityConfiguration::kReadExecute:
    case PageAccessibilityConfiguration::kReadExecuteProtected:
      return PROT_READ | PROT_EXEC;
    case PageAccessibilityConfiguration::kReadWriteExecute:
    case PageAccessibilityConfiguration::kReadWriteExecuteProtected:
    case PageAccessibilityConfiguration::kInaccessibleWillJitLater:
      return PROT_READ | PROT_WRITE | PROT_EXEC;
  }
  return PROT_READ | PROT_WRITE;
}

}  // namespace internal
}  // namespace partition_alloc

// ---------------------------------------------------------------------------
// POSIX calls newlib/devkitA64 do not provide.
// ---------------------------------------------------------------------------
extern "C" {

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
      return 3;  // Horizon exposes 3 application cores
    case 40:  // _SC_NPROCESSORS_CONF
      return 3;
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
  if (name && len) {
    name[0] = '\0';
  }
  return 0;
}

}  // extern "C"

// ---------------------------------------------------------------------------
// C runtime hooks: crti.o/crtbegin.o are not part of our link line (we link
// -nostdlib), but libnx's startup still references these.
// ---------------------------------------------------------------------------
extern "C" void _init(void) {}
extern "C" void _fini(void) {}

// ---------------------------------------------------------------------------
// base::debug::StackTrace::OutputToStreamWithPrefixImpl
// The Chromium build ships this only for the "streamed stack trace" platforms;
// for the Switch bring-up we simply emit nothing.
// ---------------------------------------------------------------------------
#include <ostream>

#include "base/debug/stack_trace.h"

namespace base::debug {

void StackTrace::OutputToStreamWithPrefixImpl(std::ostream* os,
                                              cstring_view prefix_string) const {
  (void)os;
  (void)prefix_string;
}

}  // namespace base::debug
