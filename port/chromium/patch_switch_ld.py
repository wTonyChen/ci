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

    # --- 2) Move .rodata into the writable (data) segment -------------------
    # devkitA64's libsysbase (iosupport.o) has a const stdio devoptab
    # (dotab_stdnull) whose pointer fields carry R_AARCH64_RELATIVE relocations.
    # libnx's __nx_dynamic applies those relocations by writing to the target
    # address, and hbloader maps the NRO's *rodata* segment read-only -- so the
    # write faults (that is the AMS FATAL inside __nx_dynamic). Keep .rodata in
    # the data segment instead: libnx reads __bss_start__/__bss_end__ around the
    # data segment, so the block is placed after .main.tls and before those
    # symbols, which keeps BSS clearing away from it.
    rodata_block_start = src.find("\t.rodata :\n")
    if rodata_block_start < 0:
        sys.stderr.write("warning: .rodata block not found\n")
    else:
        rodata_block_end = src.find("} :rodata", rodata_block_start)
        if rodata_block_end < 0:
            sys.stderr.write("warning: .rodata block end not found\n")
        else:
            src = (src[:rodata_block_start] +
                   src[rodata_block_end + len("} :rodata\n"):])

        # Insert right after .data and BEFORE .tdata/.tbss/.bss: BSS must stay at
        # the end of the segment (a NOBITS section in the middle of a PT_LOAD
        # would make elf2nro compute BssSize == 0 and the loader would not map
        # the zero-fill region correctly).
        anchor = "\t.tdata :\n"
        if anchor in src:
            moved = (
                "\t/* Chromium-NX: writable, see port/chromium/patch_switch_ld.py */\n"
                "\t.rodata :\n"
                "\t{\n"
                "\t\t*(.rodata .rodata.* .gnu.linkonce.r.*)\n"
                "\t\t. = ALIGN(8);\n"
                "\t} :data\n\n")
            src = src.replace(anchor, moved + anchor, 1)
            sys.stderr.write("moved .rodata into the data segment (before bss)\n")
        else:
            sys.stderr.write("warning: .tdata anchor not found\n")

    open(sys.argv[2], "w").write(src)
    sys.stderr.write("wrote %s\n" % sys.argv[2])
    return 0


if __name__ == "__main__":
    sys.exit(main())
