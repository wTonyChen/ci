#!/usr/bin/env python3
"""Adapt libnx's stock switch.ld for the Chromium-NX link.

Usage: patch_switch_ld.py <input switch.ld> <output switch.ld>

Two transformations:

1. PT_TLS: lld refuses to link objects with STT_TLS symbols unless the script
   declares a PT_TLS segment. libnx defines .tdata/.tbss/.main.tls but never
   emits a TLS program header. (Kept for the lld fallback; harmless with GNU ld.)

2. .rodata into the writable segment: devkitA64's libsysbase has a const stdio
   devoptab (dotab_stdnull) whose pointer fields carry R_AARCH64_RELATIVE
   relocations while living in .rodata. libnx's __nx_dynamic applies those
   relocations by writing to the target address; if the page is read-only that
   write faults, which is exactly the AMS FATAL we saw inside __nx_dynamic.
   Mapping .rodata into the data segment makes the relocation write legal.
"""
import sys


def main() -> int:
    if len(sys.argv) != 3:
        sys.stderr.write(__doc__)
        return 2

    src = open(sys.argv[1]).read()

    # The lld-specific PT_TLS hack is gone: we link with devkitA64's GNU ld via
    # switch.specs, and stuffing a PT_TLS phdr into the script makes GNU ld emit
    # overlapping LMAs. GNU ld handles TLS without an explicit program header.

    # --- 2) Make the whole rodata PT_LOAD writable -------------------------
    # devkitA64's libsysbase has a const stdio devoptab (dotab_stdnull) whose
    # pointer fields carry R_AARCH64_RELATIVE relocations while living in
    # .rodata. libnx's __nx_dynamic applies those relocations by writing to the
    # target address; if the page is read-only that write faults (that was the
    # AMS FATAL inside __nx_dynamic). Marking the rodata segment RW keeps the
    # section layout untouched (moving .rodata into the data segment made the
    # PT_LOADs overlap, which hbloader did not survive).
    for old, new in (
        ("\trodata PT_LOAD FLAGS(4) /* Read */;",
         "\trodata PT_LOAD FLAGS(6) /* Read | Write (Chromium-NX) */;"),
        ("\trodata PT_LOAD FLAGS(4);",
         "\trodata PT_LOAD FLAGS(6);"),
    ):
        if old in src:
            src = src.replace(old, new, 1)
            sys.stderr.write("rodata segment is now writable\n")
            break
    else:
        sys.stderr.write("warning: rodata PHDR line not found\n")

    open(sys.argv[2], "w").write(src)
    sys.stderr.write("wrote %s\n" % sys.argv[2])
    return 0


if __name__ == "__main__":
    sys.exit(main())
