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
patch("build/config/BUILDCONFIG.gn",
      "default_compiler_configs = [\n",
      "default_compiler_configs = [\n"
      "  if (!is_clang) {\n"
      "    \"//:switch_config\",\n"
      "  },\n",
      "//:switch_config")

print("all chromium patches applied")
