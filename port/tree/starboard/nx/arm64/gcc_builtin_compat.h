#ifndef GCC_BUILTIN_COMPAT_H_
#define GCC_BUILTIN_COMPAT_H_
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 14)
#define __builtin_clzg(x, y) \
  _Generic((x), \
    unsigned int: __builtin_clz(x), \
    unsigned long: __builtin_clzl(x), \
    unsigned long long: __builtin_clzll(x), \
    default: __builtin_clz((unsigned int)(x)) \
  )
#define __builtin_popcountg(x) \
  _Generic((x), \
    unsigned int: __builtin_popcount(x), \
    unsigned long: __builtin_popcountl(x), \
    unsigned long long: __builtin_popcountll(x), \
    default: __builtin_popcount((unsigned int)(x)) \
  )
#endif
#endif
