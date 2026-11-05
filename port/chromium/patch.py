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

print("all chromium patches applied")
