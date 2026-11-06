// Nintendo Switch: newlib has no <sys/prctl.h> and Horizon has no prctl().
// perfetto's base/utils.cc (Linux path) uses it for thread names and for the
// aarch64 MTE / tagged-address probes; the shim in shims/posix_shim.cc answers
// "not supported" for everything except thread naming, which is a no-op.
#ifndef FAKE_SYS_PRCTL_H_
#define FAKE_SYS_PRCTL_H_

// The full PR_* table lives in our <linux/prctl.h>; glibc puts it in
// <sys/prctl.h>, so forward it here too.
#include <linux/prctl.h>

#ifndef PR_SET_NAME
#define PR_SET_NAME 15
#endif
#ifndef PR_GET_NAME
#define PR_GET_NAME 16
#endif
#ifndef PR_SET_PDEATHSIG
#define PR_SET_PDEATHSIG 1
#endif
#ifndef PR_SET_TAGGED_ADDR_CTRL
#define PR_SET_TAGGED_ADDR_CTRL 55
#endif
#ifndef PR_GET_TAGGED_ADDR_CTRL
#define PR_GET_TAGGED_ADDR_CTRL 56
#endif
#ifndef PR_TAGGED_ADDR_ENABLE
#define PR_TAGGED_ADDR_ENABLE (1UL << 0)
#endif
#ifndef PR_MTE_TCF_SHIFT
#define PR_MTE_TCF_SHIFT 1
#endif
#ifndef PR_MTE_TCF_NONE
#define PR_MTE_TCF_NONE (0UL << PR_MTE_TCF_SHIFT)
#endif
#ifndef PR_MTE_TCF_SYNC
#define PR_MTE_TCF_SYNC (1UL << PR_MTE_TCF_SHIFT)
#endif
#ifndef PR_MTE_TCF_ASYNC
#define PR_MTE_TCF_ASYNC (2UL << PR_MTE_TCF_SHIFT)
#endif
#ifndef PR_MTE_TCF_MASK
#define PR_MTE_TCF_MASK (3UL << PR_MTE_TCF_SHIFT)
#endif
#ifndef PR_MTE_TAG_SHIFT
#define PR_MTE_TAG_SHIFT 3
#endif
#ifndef PR_MTE_TAG_MASK
#define PR_MTE_TAG_MASK (0xffffUL << PR_MTE_TAG_SHIFT)
#endif

#ifdef __cplusplus
extern "C" {
#endif

int prctl(int option, ...);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_PRCTL_H_
