// Nintendo Switch: newlib has no <sys/sysinfo.h> and Horizon has no sysinfo().
// perfetto's android_utils.cc includes it on the Linux code path.
#ifndef FAKE_SYS_SYSINFO_H_
#define FAKE_SYS_SYSINFO_H_

#include <stdint.h>

struct sysinfo {
  long uptime;
  unsigned long loads[3];
  unsigned long totalram;
  unsigned long freeram;
  unsigned long sharedram;
  unsigned long bufferram;
  unsigned long totalswap;
  unsigned long freeswap;
  unsigned short procs;
  unsigned long totalhigh;
  unsigned long freehigh;
  unsigned int mem_unit;
  char _f[20 - 2 * sizeof(long) - sizeof(int)];
};

// Inline stub: returning -1 makes callers fall back to their error path and
// avoids needing a shim object in the Chromium build graph.
#ifdef __cplusplus
extern "C" {
#endif
static inline int sysinfo(struct sysinfo* info) {
  if (info) {
    info->uptime = 0;
    info->totalram = 0;
    info->freeram = 0;
    info->mem_unit = 1;
  }
  return -1;
}
#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYS_SYSINFO_H_
