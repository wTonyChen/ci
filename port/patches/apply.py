#!/usr/bin/env python3
"""Apply Chromium-NX source patches to a Cobalt checkout (run from src/).

Kept out of the workflow YAML so the C snippets stay readable and versioned.
Each patch is idempotent and asserts on its anchor.
"""
import re
import sys

def patch(path, old, new, marker):
    s = open(path).read()
    if marker in s:
        print(f"  [skip] {path} already patched")
        return
    assert old in s, f"anchor not found in {path}"
    open(path, "w").write(s.replace(old, new, 1))
    print(f"  [ok]   {path}")

# 1) Register the nx platform (platforms.py maps platform name -> directory)
p = "starboard/build/platforms.py"
s = open(p).read()
if "nx-arm64" not in s:
    s = s.replace("    'stub': 'starboard/stub',",
                  "    'nx-arm64': 'starboard/nx/arm64',\n    'stub': 'starboard/stub',", 1)
    open(p, "w").write(s)
    print("  [ok]   starboard/build/platforms.py (registered nx-arm64)")
else:
    print("  [skip] platforms.py already patched")

# 2) platform_path.gni: starboard_target_platform is declared in this file, which is
#    NOT imported while gn parses --args, so passing it as an arg would be ignored and
#    the build would fall back to linux-x64x11. Patch the linux branch instead.
patch("starboard/build/platform_path.gni",
      'starboard_target_platform = "linux-x64x11"',
      'starboard_target_platform = "nx-arm64"   # Chromium-NX',
      "nx-arm64")

# 3) newlib hides CLOCK_THREAD_CPUTIME_ID behind _POSIX_THREAD_CPUTIME (it has no
#    per-thread CPU clock). The generic code already returns 0 when clock_gettime
#    fails, so return 0 directly on Switch.
patch("starboard/common/time.cc",
      'int64_t CurrentMonotonicThreadTime() {\n  struct timespec ts;\n  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) {\n    // This is expected to happen on some systems, like Windows.\n    return 0;\n  }\n  return ToMicroseconds(ts);\n}\n',
      'int64_t CurrentMonotonicThreadTime() {\n#if defined(__SWITCH__)\n  // Nintendo Switch: no per-thread CPU clock (newlib hides\n  // CLOCK_THREAD_CPUTIME_ID behind _POSIX_THREAD_CPUTIME).\n  return 0;\n#else\n  struct timespec ts;\n  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) {\n    // This is expected to happen on some systems, like Windows.\n    return 0;\n  }\n  return ToMicroseconds(ts);\n#endif\n}\n',
      "__SWITCH__")

# 4) newlib has no <sys/resource.h> priority API (PRIO_PROCESS, setpriority).
#    Map the Starboard priority onto a libnx thread priority instead.
patch("starboard/common/thread_platform_base.cc",
      '#include "starboard/common/thread.h"\n',
      '#include "starboard/common/thread.h"\n#if defined(__SWITCH__)\n#include <switch.h>\n#endif\n',
      "#include <switch.h>")
patch("starboard/common/thread_platform_base.cc",
      'bool SetCurrentThreadPriority(ThreadPriority priority) {\n  // setpriority returns 0 on success and -1 on failure. The default nice value\n  // is 0. See https://linux.die.net/man/2/setpriority\n  return setpriority(PRIO_PROCESS, /*who=*/0,\n                     ThreadPriorityToNiceValue(priority)) == 0;\n}\n\n',
      'bool SetCurrentThreadPriority(ThreadPriority priority) {\n#if defined(__SWITCH__)\n  // libnx thread priorities: lower value = higher priority, 0x2C is the default\n  // for user threads and the useful range is about 0x20..0x3F. Starboard hands us\n  // a POSIX nice value (lower = higher priority), so map it onto that range.\n  int sw_priority = 0x2C + ThreadPriorityToNiceValue(priority) / 5;\n  if (sw_priority < 0x20) sw_priority = 0x20;\n  if (sw_priority > 0x3F) sw_priority = 0x3F;\n  return R_SUCCEEDED(\n      svcSetThreadPriority(threadGetCurHandle(), (u32)sw_priority));\n#else\n  // setpriority returns 0 on success and -1 on failure. The default nice value\n  // is 0. See https://linux.die.net/man/2/setpriority\n  return setpriority(PRIO_PROCESS, /*who=*/0,\n                     ThreadPriorityToNiceValue(priority)) == 0;\n#endif\n}\n\n',
      "svcSetThreadPriority")

# 5) newlib's <string.h> declares strerror_r with the GNU signature (returns
#    char*) whenever _GNU_SOURCE is set -- and this port sets it. Chromium and
#    partition_alloc instead select the POSIX overload through &strerror_r,
#    which then fails to compile. Call the GNU flavour directly on Switch.
STRERROR_CALL = re.compile(
    r'(?m)^(?P<indent>[ \t]*)wrap_posix_strerror_r\(&strerror_r, err, buf, len\);')

STRERROR_NEW = r'''%(i)s#if defined(__SWITCH__)
%(i)s  // newlib's strerror_r is the GNU flavour (returns char*), so call it
%(i)s  // directly instead of resolving the POSIX overload through a pointer.
%(i)s  char* rc = strerror_r(err, buf, len);
%(i)s  if (rc != buf) {
%(i)s    buf[0] = '\0';
%(i)s    strncat(buf, rc, len - 1);
%(i)s  }
%(i)s#else
%(i)swrap_posix_strerror_r(&strerror_r, err, buf, len);
%(i)s#endif'''

def patch_strerror(path):
    s = open(path).read()
    if "__SWITCH__" in s:
        print(f"  [skip] {path} already patched")
        return
    m = STRERROR_CALL.search(s)
    assert m, f"strerror anchor not found in {path}"
    new = STRERROR_NEW % {"i": m.group("indent")}
    s = s[:m.start()] + new + s[m.end():]
    open(path, "w").write(s)
    print(f"  [ok]   {path}")

patch_strerror("base/allocator/partition_allocator/src/partition_alloc/"
               "partition_alloc_base/posix/safe_strerror.cc")
patch_strerror("base/posix/safe_strerror.cc")

# 6) perfetto's subprocess_posix.cc uses wait4() and the Linux-only rusage
#    fields (ru_maxrss/ru_minflt/ru_majflt/ru_nvcsw/ru_nivcsw), neither of which
#    newlib has. Switch never forks, so waitpid() + zeroed stats is equivalent.
SUB = "third_party/perfetto/src/base/subprocess_posix.cc"
patch(SUB,
      "    int wait_res = PERFETTO_EINTR(wait4(pid, &pid_stat, 0, &usg));\n",
      "    (void)usg;\n"
      "#if defined(__SWITCH__)\n"
      "    int wait_res = PERFETTO_EINTR(waitpid(pid, &pid_stat, 0));\n"
      "#else\n"
      "    int wait_res = PERFETTO_EINTR(wait4(pid, &pid_stat, 0, &usg));\n"
      "#endif\n",
      "int wait_res = PERFETTO_EINTR(waitpid")
patch(SUB,
      "    rusage->cpu_utime_ms = tv_to_ms(usg.ru_utime);\n",
      "#if defined(__SWITCH__)\n"
      "    // newlib's struct rusage has none of the Linux accounting fields.\n"
      "    rusage->cpu_utime_ms = 0;\n"
      "    rusage->cpu_stime_ms = 0;\n"
      "    rusage->max_rss_kb = 0;\n"
      "    rusage->min_page_faults = 0;\n"
      "    rusage->maj_page_faults = 0;\n"
      "    rusage->vol_ctx_switch = 0;\n"
      "    rusage->invol_ctx_switch = 0;\n"
      "#else\n"
      "    rusage->cpu_utime_ms = tv_to_ms(usg.ru_utime);\n",
      "newlib's struct rusage has none")
patch(SUB,
      "    rusage->invol_ctx_switch = static_cast<uint32_t>(usg.ru_nivcsw);\n",
      "    rusage->invol_ctx_switch = static_cast<uint32_t>(usg.ru_nivcsw);\n"
      "#endif\n",
      "usg.ru_nivcsw);\n#endif")

# 7) partition_alloc enables aarch64 MTE with clang-only flags
#    (-Xclang -target-feature -Xclang +mte) when it sees linux+arm64, without
#    checking is_clang. We are gcc, so gate both places on is_clang.
PA_BUILD = ("base/allocator/partition_allocator/src/partition_alloc/"
            "BUILD.gn")
patch(PA_BUILD,
      'if (current_cpu == "arm64" &&\n'
      '        (is_linux || is_chromeos || is_android || is_fuchsia)) {',
      'if (is_clang && current_cpu == "arm64" &&\n'
      '        (is_linux || is_chromeos || is_android || is_fuchsia)) {',
      "is_clang && current_cpu")
patch(PA_BUILD,
      'if ((is_android || is_linux) && target_cpu == "arm64") {',
      'if (is_clang && (is_android || is_linux) && target_cpu == "arm64") {',
      "is_clang && (is_android")

# 8) partition_alloc/stack/stack.cc uses pthread_getattr_np(), which newlib does
#    not provide. On Switch the stack is a single contiguous region, so the
#    current frame address is a good enough upper bound for stack scanning.
patch("base/allocator/partition_allocator/src/partition_alloc/stack/stack.cc",
      "#elif PA_BUILDFLAG(IS_POSIX) || PA_BUILDFLAG(IS_FUCHSIA)\n\n"
      "void* GetStackTop() {\n"
      "  pthread_attr_t attr;\n",
      "#elif defined(__SWITCH__)\n\n"
      "void* GetStackTop() {\n"
      "  // newlib has no pthread_getattr_np(); the Switch stack is contiguous,\n"
      "  // so the current frame address is a good enough upper bound.\n"
      "  return __builtin_frame_address(0);\n"
      "}\n\n"
      "#elif PA_BUILDFLAG(IS_POSIX) || PA_BUILDFLAG(IS_FUCHSIA)\n\n"
      "void* GetStackTop() {\n"
      "  pthread_attr_t attr;\n",
      "#elif defined(__SWITCH__)\n\nvoid* GetStackTop()")

# 9) partition_alloc's page allocator chooses its access flags through a GNU
#    indirect function (__attribute__((ifunc))) when the target advertises
#    BTI/MTE. devkitA64's GCC does not support ifunc on a static bare-metal
#    target ("'ifunc' is not supported on this target"), so take the plain
#    non-MTE path on Switch.
PAGE_ALLOC = ("base/allocator/partition_allocator/src/partition_alloc/"
              "page_allocator_internals_posix.cc")
if "__SWITCH__" in open(PAGE_ALLOC).read():
    print(f"  [skip] {PAGE_ALLOC} already patched")
else:
    src = open(PAGE_ALLOC).read()
    src, n = re.subn(
        r"#if defined\(NEEDS_HANDLING_OF_HW_CAPABILITIES\)(\s*\n\s*__attribute__\(\(ifunc)",
        r"#if defined(NEEDS_HANDLING_OF_HW_CAPABILITIES) && !defined(__SWITCH__)\1",
        src, count=1)
    assert n == 1, "ifunc anchor not found in " + PAGE_ALLOC
    open(PAGE_ALLOC, "w").write(src)
    print(f"  [ok]   {PAGE_ALLOC}")

def guard_line(path, line_text):
    """Wrap one exact source line in "#if !defined(__SWITCH__)" (indent-preserving)."""
    src = open(path).read()
    if re.search(r"(?m)^[ \t]*#if !defined\(__SWITCH__\)[ \t]*\n[ \t]*"
                 + re.escape(line_text), src):
        print(f"  [skip] {path} already patched ({line_text.strip()})")
        return
    pat = re.compile(r"(?m)^([ \t]*)" + re.escape(line_text) + r"[ \t]*$")
    m = pat.search(src)
    assert m, f"line not found in {path}: {line_text}"
    ind = m.group(1)
    rep = f"{ind}#if !defined(__SWITCH__)\n{ind}{line_text}\n{ind}#endif"
    open(path, "w").write(src[:m.start()] + rep + src[m.end():])
    print(f"  [ok]   {path} ({line_text.strip()})")

# 10) newlib's struct dirent has no d_off/d_reclen (Cobalt's modular ABI wrappers
#     assume the Linux layout). Fill nothing for those two fields on Switch.
DIRWRAP = ("starboard/shared/modular/"
           "starboard_layer_posix_directory_abi_wrappers.cc")
for _line in ("musl_entry->d_off = entry.d_off;",
              "musl_entry->d_reclen = entry.d_reclen;",
              "dirp->musl_dir_entry->d_off = result_platform->d_off;",
              "dirp->musl_dir_entry->d_reclen = result_platform->d_reclen;"):
    guard_line(DIRWRAP, _line)

# 11) Cobalt's modular fcntl ABI wrappers assume the musl (Linux) O_*/F_* values
#     and static_assert them. newlib's values differ (O_CREAT is 0x200, not
#     0100, ...), so the asserts cannot hold on Switch. They are documentation
#     only; skip them here and note that the modular ABI layer deserves a
#     revisit once the build links (see RESUME.md "modular ABI" note).
FCNTL_H = ("starboard/shared/modular/"
           "starboard_layer_posix_fcntl_abi_wrappers.h")
if "__SWITCH__" in open(FCNTL_H).read():
    print(f"  [skip] {FCNTL_H} already patched")
else:
    src = open(FCNTL_H).read()
    src, n1 = re.subn(r"(?m)^static_assert\(O_CREAT ==",
                      "#if !defined(__SWITCH__)\nstatic_assert(O_CREAT ==", src, count=1)
    src, n2 = re.subn(r"(?m)^#ifdef __cplusplus\nextern \"C\" \{",
                      "#endif  // !__SWITCH__\n#ifdef __cplusplus\nextern \"C\" {",
                      src, count=1)
    assert n1 == 1 and n2 == 1, "fcntl wrapper header anchors not found"
    open(FCNTL_H, "w").write(src)
    print(f"  [ok]   {FCNTL_H} (static_asserts skipped on Switch)")

# 12) abseil's failure_signal_handler installs a SA_SIGINFO handler and stores
#     it in action.sa_sigaction. newlib's struct sigaction has no sa_sigaction
#     member (that variant only exists on RTEMS builds), so make the installer a
#     no-op on Switch: Horizon homebrew has no useful POSIX crash signal path.
FSH = ("third_party/abseil-cpp/absl/debugging/failure_signal_handler.cc")
if "__SWITCH__" in open(FSH).read():
    print(f"  [skip] {FSH} already patched")
else:
    src = open(FSH).read()
    sig = ("void InstallFailureSignalHandler(const FailureSignalHandlerOptions& "
           "options) {")
    assert sig in src, "failure_signal_handler anchor not found"
    i = src.index(sig) + len(sig)
    depth, j = 1, i
    while depth:
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
        j += 1
    body = src[i:j - 1]
    patched = (src[:i]
               + "\n#if defined(__SWITCH__)\n"
               + "  // newlib's struct sigaction has no sa_sigaction, and Horizon "
                 "homebrew has no useful POSIX crash-signal path.\n"
               + "  (void)options;\n  return;\n#else\n"
               + body
               + "\n#endif  // __SWITCH__\n"
               + src[j - 1:])
    open(FSH, "w").write(patched)
    print(f"  [ok]   {FSH} (whole body skipped on Switch)")

def patch_function_body(path, signature, switch_body, marker):
    """Wrap a whole function body in #if defined(__SWITCH__) ... #else ... #endif."""
    src = open(path).read()
    if marker in src:
        print(f"  [skip] {path} already patched ({signature.strip()[:40]})")
        return
    assert signature in src, f"signature not found in {path}: {signature[:60]}"
    i = src.index(signature) + len(signature)
    depth, j = 1, i
    while depth:
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
        j += 1
    body = src[i:j - 1]
    out = (src[:i] + "\n#if defined(__SWITCH__)\n" + switch_body
           + "\n#else\n" + body + "\n#endif  // __SWITCH__\n" + src[j - 1:])
    open(path, "w").write(out)
    print(f"  [ok]   {path} ({signature.strip()[:40]})")


# 13) newlib's siginfo_t only has si_signo/si_code/si_value: no si_pid/si_uid/
#     si_addr/... Cobalt's modular signal wrapper reads the full Linux layout.
#     Signals are not used on Switch, so translate just the two common fields.
patch_function_body(
    "starboard/shared/modular/starboard_layer_posix_signal_abi_wrappers.cc",
    ("void PlatformToMuslSiginfo(const siginfo_t* platform_si,\n"
     "                           musl_siginfo_t* musl_si) {"),
    ("  if (!platform_si || !musl_si) {\n"
     "    return;\n"
     "  }\n"
     "  // newlib's siginfo_t has only si_signo/si_code/si_value; keep those.\n"
     "  memset(musl_si, 0, sizeof(*musl_si));\n"
     "  musl_si->musl_si_signo = platform_si->si_signo;\n"
     "  musl_si->musl_si_code = platform_si->si_code;\n"),
    "#if defined(__SWITCH__)\n  // newlib's siginfo_t has only")

print("all port patches applied")
