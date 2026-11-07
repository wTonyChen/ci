#!/usr/bin/env python3
"""Wrap devkitA64's GNU ld for the Chromium link command.

Chromium's Linux toolchain hands the linker a pile of flags that only lld
understands (--icf=all, --color-diagnostics, --undefined-version, ...). ld.bfd
dies on the first one. Rather than patching every Chromium build config, this
wrapper drops the lld-only options and execs the real ld.bfd.

Wired in via `-fuse-ld=<this file>` from //:nx_link.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.dirname(HERE)          # .../src
REAL_LD = os.path.join(SRC, "third_party", "devkitA64", "aarch64-none-elf",
                       "bin", "ld")

# Options lld knows but GNU ld does not.
DROP = (
    "--icf=all",
    "--icf=none",
    "--color-diagnostics",
    "--undefined-version",
    "--no-call-graph-profile-sort",
    "-z,pack-relative-relocs",
    "--build-id=fast",
    "--as-needed",       # bfd supports it, but keep the link simple/predictable
    "--thinlto-index-only",
)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith(DROP)]
    if not os.path.exists(REAL_LD):
        sys.stderr.write("ld_wrapper: %s not found\n" % REAL_LD)
        return 127
    os.execv(REAL_LD, [REAL_LD] + args)
    return 127


if __name__ == "__main__":
    sys.exit(main())
