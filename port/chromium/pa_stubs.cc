// partition_alloc compiles some translation units with HAS_MEMORY_TAGGING on
// (Arm MTE) even though the Switch target has no MTE: the buildflag ends up true
// for these targets, so the MTE helper calls are emitted but tagging.cc -- which
// would define them for MTE hardware -- is compiled with them off. On a
// non-MTE system they are no-ops, so provide the identity versions here.
//
// The declarations are guarded by PA_BUILDFLAG(HAS_MEMORY_TAGGING) in
// partition_alloc/tagging.h inside namespace partition_alloc::internal.

#include <cstddef>
#include <cstdint>

namespace partition_alloc::internal {

void* RemaskPointerInternal(void* ptr) {
  return ptr;
}

void* TagMemoryRangeIncrementInternal(void* ptr, size_t /*size*/) {
  return ptr;
}

void* TagMemoryRangeRandomlyInternal(void* ptr, size_t /*size*/,
                                     uint64_t /*mask*/) {
  return ptr;
}

}  // namespace partition_alloc::internal

// ---------------------------------------------------------------------------
// Rust <-> C++ allocator bridge.
//
// Chromium's Rust global allocator calls these four C++ functions. The real
// implementation lives in //build/rust/allocator, which is only linked together
// with rustc-driven targets; when a C++ executable links //base, the symbols are
// left undefined. Provide them on top of the devkitA64 allocator.
// Declarations: build/rust/allocator/allocator_impls.h
// ---------------------------------------------------------------------------
#include <cstdlib>
#include <malloc.h>
#include <cstring>

namespace rust_allocator_internal {

unsigned char* alloc(size_t size, size_t align) {
  void* p = nullptr;
  if (align <= sizeof(void*)) {
    p = malloc(size == 0 ? 1 : size);
  } else {
    p = memalign(align, size == 0 ? 1 : size);
  }
  return static_cast<unsigned char*>(p);
}

void dealloc(unsigned char* p, size_t /*size*/, size_t /*align*/) {
  free(p);
}

unsigned char* realloc(unsigned char* p, size_t old_size, size_t align,
                       size_t new_size) {
  if (p == nullptr) {
    return alloc(new_size, align);
  }
  if (new_size == 0) {
    dealloc(p, old_size, align);
    return nullptr;
  }
  // devkitA64's realloc cannot honour a stronger alignment; fall back to
  // allocate-copy-free when the caller asked for one.
  if (align <= sizeof(void*)) {
    return static_cast<unsigned char*>(realloc(p, new_size));
  }
  unsigned char* q = alloc(new_size, align);
  if (q != nullptr) {
    std::memcpy(q, p, old_size < new_size ? old_size : new_size);
    dealloc(p, old_size, align);
  }
  return q;
}

unsigned char* alloc_zeroed(size_t size, size_t align) {
  unsigned char* p = alloc(size, align);
  if (p != nullptr) {
    std::memset(p, 0, size);
  }
  return p;
}

}  // namespace rust_allocator_internal
