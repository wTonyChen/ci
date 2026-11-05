// Nintendo Switch: newlib has no <link.h> (glibc uses it to expose ElfW()).
// partition_alloc's stack_trace_posix.cc includes it "for ElfW() macro", so
// provide the small subset it actually needs.
#ifndef FAKE_LINK_H_
#define FAKE_LINK_H_

#include <elf.h>

#ifndef SELFMAG
#define SELFMAG 4
#endif
#ifndef ElfW
#define ElfW(type) Elf64_##type
#endif

#endif  // FAKE_LINK_H_
