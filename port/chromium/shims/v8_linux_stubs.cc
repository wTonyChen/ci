// Stubs for the Linux-isms V8's //src/base pulls in.
//
// V8 is built with target_os="linux" (that is the closest Chromium-supported OS
// in this tree), so a handful of glibc/Linux-only entry points show up at link
// time. Horizon has none of them; the callers are all in code paths that are
// allowed to fail on an unsupported platform.
#include <cerrno>
#include <cstddef>

#include <sys/mman.h>

extern "C" {

// V8's src/base/debug/stack_trace_posix.cc. We never install a signal handler
// and never print V8 stack traces on device, so "empty" is a fine answer.
int backtrace(void** buffer, int size) {
  (void)buffer;
  (void)size;
  return 0;
}

char** backtrace_symbols(void* const* buffer, int size) {
  (void)buffer;
  (void)size;
  return nullptr;
}

void backtrace_symbols_fd(void* const* buffer, int size, int fd) {
  (void)buffer;
  (void)size;
  (void)fd;
}

// mremap(): V8 uses it for RemapShared()/DiscardSystemPages(). Failing with
// ENOSYS makes RemapShared() report nullptr, which its callers already handle.
void* mremap(void* old_address, size_t old_size, size_t new_size, int flags, ...) {
  (void)old_address;
  (void)old_size;
  (void)new_size;
  (void)flags;
  errno = ENOSYS;
  return MAP_FAILED;
}

// newlib has no getrusage(): V8 uses it only for peak-RSS reporting.
struct rusage;
int getrusage(int who, struct rusage* usage) {
  (void)who;
  (void)usage;
  errno = ENOSYS;
  return -1;
}

}  // extern "C"
