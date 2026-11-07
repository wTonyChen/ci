#!/usr/bin/env python3
"""Turn libnx's stock switch.ld into an lld-compatible one (adds PT_TLS).

Usage: patch_switch_ld.py <input switch.ld> <output switch.ld>

Why: lld refuses to link objects that carry STT_TLS symbols unless the linker
script declares a PT_TLS segment. libnx's script defines .tdata/.tbss and
reserves .main.tls for the main thread, but never emits a TLS program header
(GNU ld is happy without one). This keeps every libnx symbol intact and only
adds the phdr plus the segment mappings.
"""
import sys


def main() -> int:
    if len(sys.argv) != 3:
        sys.stderr.write(__doc__)
        return 2
    src = open(sys.argv[1]).read()

    if "PT_TLS" in src:
        sys.stderr.write("already has PT_TLS, copying as-is\n")
        open(sys.argv[2], "w").write(src)
        return 0

    # 1) declare the TLS program header right after the data segment
    for anchor in ("\tdata   PT_LOAD FLAGS(6) /* Read | Write */;",
                   "\tdata PT_LOAD FLAGS(6)"):
        if anchor in src:
            src = src.replace(
                anchor,
                anchor + "\n\ttls    PT_TLS FLAGS(6) /* TLS for lld */;", 1)
            break
    else:
        sys.stderr.write("could not find PHDRS data line\n")
        return 1

    # 2) attach the TLS sections to the new segment, right after their block
    def attach(text: str, name: str) -> str:
        idx = text.find(name)
        if idx < 0:
            return text
        brace = text.find("} :data", idx)
        if brace >= 0:
            return text[:brace] + "} :data :tls" + text[brace + len("} :data"):]
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

    open(sys.argv[2], "w").write(src)
    print("wrote %s with PT_TLS" % sys.argv[2])
    return 0


if __name__ == "__main__":
    sys.exit(main())
