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

    # --- 1) TLS program header (only if not already present) -----------------
    if "PT_TLS" in src:
        sys.stderr.write("switch.ld already declares PT_TLS\n")
    else:
        for anchor in ("\tdata   PT_LOAD FLAGS(6) /* Read | Write */;",
                       "\tdata PT_LOAD FLAGS(6)"):
            if anchor in src:
                src = src.replace(
                    anchor,
                    anchor + "\n\ttls    PT_TLS FLAGS(6) /* Chromium-NX: for lld */;",
                    1)
                break
        else:
            sys.stderr.write("warning: PHDRS data line not found\n")

        def attach(text: str, name: str) -> str:
            idx = text.find(name)
            if idx < 0:
                return text
            brace = text.find("} :data", idx)
            if brace >= 0:
                return (text[:brace] + "} :data :tls" +
                        text[brace + len("} :data"):])
            brace = text.find("\n\t}", idx)
            if brace >= 0:
                return (text[:brace] + "\n\t} :data :tls" +
                        text[brace + len("\n\t}"):])
            return text

        for sec in ("\t.tdata ", "\t.tbss ", "\t.main.tls "):
            before = src
            src = attach(src, sec)
            if src == before:
                sys.stderr.write("warning: could not attach %s\n" % sec.strip())

    # --- 2) .rodata into the writable segment -------------------------------
    marker = "\t.rodata :\n"
    idx = src.find(marker)
    if idx < 0:
        sys.stderr.write("warning: .rodata block not found\n")
    else:
        end = src.find("} :rodata", idx)
        if end < 0:
            sys.stderr.write("warning: .rodata block end not found\n")
        else:
            src = (src[:end] +
                   "} :data  /* Chromium-NX: writable, see patch_switch_ld.py */" +
                   src[end + len("} :rodata"):])
            sys.stderr.write("mapped .rodata into the writable segment\n")

    open(sys.argv[2], "w").write(src)
    sys.stderr.write("wrote %s\n" % sys.argv[2])
    return 0


if __name__ == "__main__":
    sys.exit(main())
