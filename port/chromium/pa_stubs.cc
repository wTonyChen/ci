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
