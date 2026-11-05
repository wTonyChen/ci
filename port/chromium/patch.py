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
#    toolchain. Guarded with !is_clang so the host (clang_x64) toolchain keeps
#    its own configs and never sees __SWITCH__/fake POSIX headers.
# NOTE: `if` entries inside a list literal are NOT accepted by Chromium's gn
# ("Unexpected token 'if'"), so append after the list instead.
patch("build/config/BUILDCONFIG.gn",
      '  "//build/config:feature_flags",\n]\n',
      '  "//build/config:feature_flags",\n]\n'
      '\n'
      '# Chromium-NX: Switch target configs (defines, fake POSIX headers,\n'
      '# force-included compat header). Guarded so the host toolchain, which runs\n'
      '# with is_clang=true, never sees them.\n'
      'if (!is_clang) {\n'
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
STRERROR_CALL = re.compile(
    r'(?m)^(?P<indent>[ \t]*)wrap_posix_strerror_r\(&strerror_r, err, buf, len\);')

STRERROR_NEW = r'''%(i)s#if defined(__SWITCH__)
%(i)s  // newlib's strerror_r is the GNU flavour (returns char*).
%(i)s  char* rc = strerror_r(err, buf, len);
%(i)s  if (rc != buf) {
%(i)s    buf[0] = '\0';
%(i)s    strncat(buf, rc, len - 1);
%(i)s  }
%(i)s#else
%(i)swrap_posix_strerror_r(&strerror_r, err, buf, len);
%(i)s#endif'''


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

print("all chromium patches applied")
