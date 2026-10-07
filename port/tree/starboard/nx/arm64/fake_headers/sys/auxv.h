#ifndef FAKE_SYS_AUXV_H_
#define FAKE_SYS_AUXV_H_
#ifdef __cplusplus
extern "C" {
#endif
unsigned long getauxval(unsigned long type);
#ifdef __cplusplus
}
#endif
#endif
