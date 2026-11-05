// Nintendo Switch: newlib has no <linux/prctl.h>. Cobalt's modular prctl ABI
// wrapper includes it for the full set of PR_* operation codes. Values are the
// Linux uapi ones (asm-generic/prctl.h).
#ifndef FAKE_LINUX_PRCTL_H_
#define FAKE_LINUX_PRCTL_H_

#define PR_SET_PDEATHSIG 1
#define PR_GET_PDEATHSIG 2
#define PR_GET_DUMPABLE 3
#define PR_SET_DUMPABLE 4
#define PR_GET_KEEPCAPS 7
#define PR_SET_KEEPCAPS 8
#define PR_GET_TIMING 13
#define PR_SET_TIMING 14
#define PR_SET_NAME 15
#define PR_GET_NAME 16
#define PR_GET_TSC 25
#define PR_SET_TSC 26
#define PR_GET_TIMERSLACK 30
#define PR_SET_TIMERSLACK 29
#define PR_TASK_PERF_EVENTS_DISABLE 31
#define PR_TASK_PERF_EVENTS_ENABLE 32
#define PR_SET_PTRACER 0x59616d61
#define PR_SET_PTRACER_ANY ((unsigned long)-1)

#define PR_TIMING_STATISTICAL 0
#define PR_TIMING_TIMESTAMP 1
#define PR_TSC_ENABLE 1
#define PR_TSC_SIGSEGV 2

#define PR_SET_VMA 0x53564d41
#define PR_SET_VMA_ANON_NAME 0

#endif  // FAKE_LINUX_PRCTL_H_
