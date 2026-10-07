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

print("all port patches applied")
