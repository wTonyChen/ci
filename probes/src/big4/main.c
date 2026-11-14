// big4 —— 同一份源码，把 .data 垫片放大到 ~4 MB，用来测「hbmenu 加载器是不是按尺寸崩」。
// 只改一个变量：NRO 体积。行为、面包屑、日志与 big 完全相同。
#define PAD_BYTES 4000000
#include "../big/main.c"
