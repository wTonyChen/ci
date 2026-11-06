// Nintendo Switch: newlib has no <dlfcn.h> and Horizon homebrew has no dynamic
// linker, so every dynamic-lookup entry point is a stub (see shims/posix_shim.cc).
// Chromium's base/third_party/symbolize needs the declarations to compile; at
// runtime it will simply fail to resolve symbols, which is acceptable.
#ifndef FAKE_DLFCN_H_
#define FAKE_DLFCN_H_

// glibc's <dlfcn.h> also declares Dl_info + dladdr(), used by
// base/profiler/module_cache_posix.cc.
typedef struct {
  const char* dli_fname;
  void* dli_fbase;
  const char* dli_sname;
  void* dli_saddr;
} Dl_info;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTLD_LAZY
#define RTLD_LAZY 0x0001
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 0x0002
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0x0100
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0x0000
#endif
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void*)0)
#endif
#ifndef RTLD_NEXT
#define RTLD_NEXT ((void*)-1)
#endif

void* dlopen(const char* filename, int flags);
void* dlsym(void* handle, const char* symbol);
int dlclose(void* handle);
char* dlerror(void);
int dladdr(const void* addr, void* info);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_DLFCN_H_
