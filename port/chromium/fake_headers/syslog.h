// Nintendo Switch: newlib has no <syslog.h>. base/syslog_logging.cc only needs
// the API surface; there is no syslog daemon on Horizon, so the calls are
// no-ops (base already degrades gracefully when syslog is unavailable).
#ifndef FAKE_SYSLOG_H_
#define FAKE_SYSLOG_H_

#include <stdarg.h>

#ifndef LOG_EMERG
#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7
#endif

#ifndef LOG_PID
#define LOG_PID 0x01
#define LOG_CONS 0x02
#define LOG_NDELAY 0x08
#define LOG_NOWAIT 0x10
#define LOG_PERROR 0x20
#endif

#ifndef LOG_KERN
#define LOG_KERN (0 << 3)
#define LOG_USER (1 << 3)
#define LOG_MAIL (2 << 3)
#define LOG_DAEMON (3 << 3)
#define LOG_AUTH (4 << 3)
#define LOG_SYSLOG (5 << 3)
#define LOG_LPR (6 << 3)
#define LOG_NEWS (7 << 3)
#define LOG_LOCAL7 (23 << 3)
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline void openlog(const char* ident, int option, int facility) {
  (void)ident;
  (void)option;
  (void)facility;
}
static inline void syslog(int priority, const char* format, ...) {
  (void)priority;
  (void)format;
}
static inline void closelog(void) {}
static inline int setlogmask(int mask) {
  (void)mask;
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif  // FAKE_SYSLOG_H_
