#!/usr/bin/env python3
"""Apply Chromium-NX patches to a chromium/src checkout (run from src/).

Kept as a script (not inline YAML) so the anchors are readable and reviewable.
Every patch is idempotent and asserts on its anchor.
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


# 1) Wire //:switch_config into the default config list of every *target*
#    toolchain. Guarded with current_cpu so the host (x64) toolchain keeps its
#    own configs and never sees __SWITCH__/fake POSIX headers. (`is_clang` can
#    no longer discriminate: plan A made the target clang as well.)
# NOTE: `if` entries inside a list literal are NOT accepted by Chromium's gn
# ("Unexpected token 'if'"), so append after the list instead.
patch("build/config/BUILDCONFIG.gn",
      '  "//build/config:feature_flags",\n]\n',
      '  "//build/config:feature_flags",\n]\n'
      '\n'
      '# Chromium-NX: Switch target configs (defines, fake POSIX headers,\n'
      '# force-included compat header). Guarded by current_cpu so the x64 host\n'
      '# toolchain never sees them (both toolchains are clang now).\n'
      'if (current_cpu == "arm64") {\n'
      '  default_compiler_configs += [ "//:switch_config" ]\n'
      '}\n',
      "//:switch_config")

# 2) perfetto's subprocess_posix.cc uses wait4() and the Linux-only rusage
#    fields, neither of which newlib has. Switch never forks, so waitpid() plus
#    zeroed stats is equivalent. (Same fix as the frozen Cobalt port.)
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

# 3) newlib's pthread_t is a POINTER to an incomplete struct, while glibc's is
#    an integer. partition_alloc's PlatformThreadRef::hash_value() uses
#    static_cast<uintptr_t>(id_), which is invalid for a pointer type.
PTR = ("base/allocator/partition_allocator/src/partition_alloc/"
       "partition_alloc_base/threading/platform_thread_ref.h")
patch(PTR,
      "#else\n"
      "  template <typename Hasher>\n"
      "  size_t hash_value() const {\n"
      "    return Hasher()(static_cast<uintptr_t>(id_));\n"
      "  }\n"
      "#endif\n",
      "#else\n"
      "  template <typename Hasher>\n"
      "  size_t hash_value() const {\n"
      "#if defined(__SWITCH__)\n"
      "    // newlib's pthread_t is a pointer type, not an integer.\n"
      "    return Hasher()(reinterpret_cast<uintptr_t>(id_));\n"
      "#else\n"
      "    return Hasher()(static_cast<uintptr_t>(id_));\n"
      "#endif\n"
      "  }\n"
      "#endif\n",
      "newlib's pthread_t is a pointer type")

# 4) newlib declares strerror_r with the GNU signature (returns char*) when
#    _GNU_SOURCE is set, while Chromium/partition_alloc pick the POSIX overload
#    through &strerror_r. Same fix as the Cobalt port: call the GNU flavour.
# Chromium main passes a span<char> (no explicit length):
#   wrap_posix_strerror_r(&strerror_r, err, buf);
STRERROR_CALL = re.compile(
    r'(?m)^(?P<indent>[ \t]*)wrap_posix_strerror_r\(&strerror_r, err, buf\);')

STRERROR_NEW = r'''%(i)s#if defined(__SWITCH__)
%(i)s  // newlib's strerror_r is the GNU flavour (returns char*).
%(i)s  char* rc = strerror_r(err, buf.data(), buf.size());
%(i)s  if (rc != buf.data()) {
%(i)s    buf[0] = '\0';
%(i)s    strncat(buf.data(), rc, buf.size() - 1);
%(i)s  }
%(i)s#else
%(i)swrap_posix_strerror_r(&strerror_r, err, buf);
%(i)s#endif'''


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



def patch_strerror(path):
    src = open(path).read()
    if "__SWITCH__" in src:
        print(f"  [skip] {path} already patched")
        return
    m = STRERROR_CALL.search(src)
    assert m, f"strerror anchor not found in {path}"
    new = STRERROR_NEW % {"i": m.group("indent")}
    open(path, "w").write(src[:m.start()] + new + src[m.end():])
    print(f"  [ok]   {path}")


patch_strerror("base/allocator/partition_allocator/src/partition_alloc/"
               "partition_alloc_base/posix/safe_strerror.cc")
patch_strerror("base/posix/safe_strerror.cc")

# 5) absl's failure_signal_handler installs a SA_SIGINFO handler through
#    act.sa_sigaction, a member newlib's struct sigaction does not have (that
#    union only exists in newlib's RTEMS build). Horizon has no useful POSIX
#    crash-signal path anyway, so make the installer a no-op on Switch.
patch_function_body(
    "third_party/abseil-cpp/absl/debugging/failure_signal_handler.cc",
    ("static void InstallOneFailureHandler(FailureSignalData* data,\n"
     "                                     void (*handler)(int, siginfo_t*, void*)) {"),
    "  (void)data;\n  (void)handler;\n",
    "#if defined(__SWITCH__)\n  (void)data;\n  (void)handler;")

# 6) base/profiler/stack_copier_signal.cc installs a SA_SIGINFO handler through
#    act.sa_sigaction, which newlib's struct sigaction does not have (absl got the
#    same treatment above). Horizon has no process-directed signals, so bail out
#    before touching the sigaction at all.
patch("base/profiler/stack_copier_signal.cc",
      "    action.sa_sigaction = CopyStackSignalHandler;\n",
      "#if defined(__SWITCH__)\n"
      "    // No SA_SIGINFO / thread-directed signals on Horizon.\n"
      "    return false;\n"
      "#else\n"
      "    action.sa_sigaction = CopyStackSignalHandler;\n"
      "#endif\n",
      "No SA_SIGINFO / thread-directed signals on Horizon")

# 7) newlib's struct sigaction has no sa_sigaction member (that union only exists
#    in newlib's RTEMS build). base/debug/stack_trace_posix.cc assigns to it in
#    three places and compares it in one. On Horizon the crash-dump handler
#    cannot be installed anyway, so drop those statements.
STACK_TRACE = "base/debug/stack_trace_posix.cc"


def patch_sa_sigaction(path):
    src = open(path).read()
    if "// __SWITCH__: newlib has no sa_sigaction" in src:
        print(f"  [skip] {path} already patched")
        return
    lines = src.split("\n")
    out = []
    for line in lines:
        stripped = line.strip()
        if re.match(r"^[A-Za-z_][A-Za-z0-9_]*\\.sa_sigaction\\s*=", stripped) or \
           stripped.startswith("action.sa_sigaction ="):
            indent = line[: len(line) - len(line.lstrip())]
            out.append(f"{indent}// __SWITCH__: newlib has no sa_sigaction")
            out.append(f"{indent}#if !defined(__SWITCH__)")
            out.append(line)
            out.append(f"{indent}#endif")
        elif "installed_handler.sa_sigaction != StackDumpSignalHandler" in line:
            indent = line[: len(line) - len(line.lstrip())]
            out.append(f"{indent}// __SWITCH__: newlib has no sa_sigaction")
            out.append(f"{indent}#if defined(__SWITCH__)")
            out.append(f"{indent}(void)0;")
            out.append(f"{indent}#else")
            out.append(line)
            out.append(f"{indent}#endif")
        else:
            out.append(line)
    open(path, "w").write("\n".join(out))
    print(f"  [ok]   {path} (sa_sigaction)")


patch_sa_sigaction(STACK_TRACE)

# 8) newlib's siginfo_t has only si_signo/si_code/si_value — no si_addr (nor the
#    rest of the glibc union). stack_trace_posix.cc prints the fault address.
patch("base/debug/stack_trace_posix.cc",
      "    internal::itoa_r(reinterpret_cast<intptr_t>(info->si_addr), 16, 12, buf);\n",
      "#if defined(__SWITCH__)\n"
      "    // newlib's siginfo_t has no si_addr.\n"
      "    internal::itoa_r(0, 16, 12, buf);\n"
      "#else\n"
      "    internal::itoa_r(reinterpret_cast<intptr_t>(info->si_addr), 16, 12, buf);\n"
      "#endif\n",
      "newlib's siginfo_t has no si_addr")

print("all chromium patches applied")
