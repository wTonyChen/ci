// stack_trace_stub.cc —— Switch 上没有 execinfo.h / backtrace(3)。
//
// base/debug/stack_trace_posix.cc 把 OutputToStreamWithPrefixImpl 的实现放在
// `#if defined(HAVE_BACKTRACE)` 里，而我们故意不定义 HAVE_BACKTRACE（newlib 没有
// backtrace()），于是任何链接 //base 且触发 CHECK/LOG 的程序都会缺这个符号。
// 这里给一个不打印回溯的空实现，至少让错误信息本身能出来。
#include "base/debug/stack_trace.h"

#include <ostream>

namespace base::debug {

void StackTrace::OutputToStreamWithPrefixImpl(std::ostream* os,
                                              cstring_view prefix_string) const {
  *os << prefix_string << "(stack trace unavailable on this platform)\n";
}

}  // namespace base::debug
