#!/usr/bin/env python3
"""Validate makerom's CIA title version, adapted from OpenDSP/tools/check_cia.py."""
import struct
import sys
from pathlib import Path

def version(data):
    header, _, _, cert, ticket = struct.unpack_from("<IHHII", data)
    align = lambda n: (n + 63) & ~63
    off = align(align(align(header) + cert) + ticket)
    sig = struct.unpack_from(">I", data, off)[0]
    sig_len = {0x10000:0x240,0x10001:0x140,0x10002:0x80,
               0x10003:0x240,0x10004:0x140,0x10005:0x80}[sig]
    return struct.unpack_from(">H", data, off + sig_len + 0x9c)[0]

if __name__ == "__main__":
    path, expected = sys.argv[1:]
    major, minor, micro = map(int, expected.split("."))
    value = major << 10 | minor << 4 | micro
    actual = version(Path(path).read_bytes())
    if actual != value:
        sys.exit(f"{path}: CIA title version {actual}, expected {value} ({expected})")
    print(f"{path}: title version {expected} ({actual})")
