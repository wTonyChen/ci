// Nintendo Switch: newlib has no <sys/ucontext.h>. abseil's examine_stack.cc
// includes it and, on the Linux path, reads the signal context's program
// counter from `uc_mcontext.pc` (aarch64). Provide just enough of the type.
#ifndef FAKE_SYS_UCONTEXT_H_
#define FAKE_SYS_UCONTEXT_H_

#include <stdint.h>

// Layout mirrors the Linux aarch64 mcontext_t closely enough for abseil, which
// only reads .pc and .regs[29] (frame pointer).
typedef struct {
  uint64_t fault_address;
  uint64_t regs[31];
  uint64_t sp;
  uint64_t pc;
} mcontext_t;

typedef struct ucontext_t {
  unsigned long uc_flags;
  struct ucontext_t* uc_link;
  mcontext_t uc_mcontext;
} ucontext_t;

#endif  // FAKE_SYS_UCONTEXT_H_
