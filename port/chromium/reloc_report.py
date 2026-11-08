#!/usr/bin/env python3
"""List relocations that land in RX segments (i.e. text relocations) and name
the symbol each one targets.

Usage: reloc_report.py <elf>   (run with devkitA64 binutils in PATH)
"""
import re
import subprocess
import sys


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True).stdout


def main():
    if len(sys.argv) != 2:
        sys.stderr.write(__doc__)
        return 2
    elf = sys.argv[1]

    # RX segment ranges
    rx = []
    for line in run(["readelf", "-lW", elf]).splitlines():
        m = re.match(r"\s*LOAD\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+).*?(R\s*E|RWE)\s", line)
        if m:
            off = int(m.group(1), 16)
            vaddr = int(m.group(2), 16)
            size = int(line.split()[4], 16)
            rx.append((off, vaddr, size))
    print("RX segments:", [(hex(o), hex(v), hex(s)) for o, v, s in rx])

    # symbols by address
    syms = []
    for line in run(["readelf", "-sW", elf]).splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[0].rstrip(":").isdigit():
            try:
                value = int(parts[1], 16)
            except ValueError:
                continue
            name = parts[7]
            syms.append((value, name))
    syms.sort()

    def nearest(addr):
        best = None
        for value, name in syms:
            if value <= addr:
                best = (value, name)
            else:
                break
        return best

    # relocations
    cur_off = 0
    hits = []
    for line in run(["readelf", "-rW", elf]).splitlines():
        m = re.match(r"\s*([0-9a-f]{8,})\s+([0-9a-f]{16})\s+(\S+)\s*(.*)", line)
        if not m:
            continue
        offset = int(m.group(1), 16)
        rtype = m.group(3)
        rest = m.group(4).strip()
        for off, vaddr, size in rx:
            if vaddr <= offset < vaddr + size:
                hits.append((offset, rtype, rest))
                break
    print(f"text relocations: {len(hits)}")
    seen = {}
    for offset, rtype, rest in hits:
        sym = nearest(offset)
        key = (rtype, (sym[1] if sym else "?"))
        seen[key] = seen.get(key, 0) + 1
    for (rtype, name), count in sorted(seen.items(), key=lambda kv: -kv[1])[:40]:
        print(f"  {count:5d}  {rtype:22s} {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
