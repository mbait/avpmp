"""RFFL fastfile archives (src/avp/win95/ffread.cpp, ffstdio.cpp).

Layout (little-endian, no compression):
    char   magic[4]  "RFFL"
    uint32 version   (must be 0)
    uint32 num_files
    uint32 dir_size  (bytes of directory that follow the 20-byte header)
    uint32 data_size
    directory: num_files x { uint32 offset; uint32 length; char name[] NUL }
               each entry padded so it occupies (12 + strlen(name)) & ~3 bytes
    data:      data_size bytes; offsets are relative to the start of data

Names are full game paths with backslashes, e.g. "Graphics\\Common\\foo.RIM";
lookup is case-insensitive. fastfile/ffinfo.txt maps a directory prefix to
the .ffl that holds it ("<prefix>;<file.ffl>", only lines starting with a
letter count; an uppercase first letter keeps the archive resident).
"""

import struct

from . import disc

MAGIC = b"RFFL"


def read(data):
    """Return list of (name, bytes)."""
    if disc.is_scrambled(data[:64]):
        data = disc.decode(data)
    if data[:4] != MAGIC:
        raise ValueError("not an RFFL archive")
    ver, n, dsize, datasize = struct.unpack_from("<IIII", data, 4)
    if ver != 0:
        raise ValueError("unsupported RFFL version %d" % ver)
    base = 20 + dsize
    out = []
    p = 20
    for _ in range(n):
        off, ln = struct.unpack_from("<II", data, p)
        e = data.index(b"\0", p + 8)
        name = data[p + 8:e].decode("latin-1")
        p += (12 + len(name)) & ~3
        out.append((name, data[base + off:base + off + ln]))
    return out


def write(files):
    """Build an RFFL archive from (name, bytes) pairs."""
    dirb = bytearray()
    datab = bytearray()
    for name, blob in files:
        e = name.encode("latin-1")
        entry = struct.pack("<II", len(datab), len(blob)) + e + b"\0"
        entry += b"\0" * (((12 + len(e)) & ~3) - len(entry))
        dirb += entry
        datab += blob
        datab += b"\0" * (-len(datab) % 4)
    hdr = MAGIC + struct.pack("<IIII", 0, len(files), len(dirb), len(datab))
    return bytes(hdr + dirb + datab)


def parse_ffinfo(text):
    """Parse fastfile/ffinfo.txt into [(prefix, ffl_name)]."""
    out = []
    for line in text.splitlines():
        line = line.strip()
        if not line or not line[0].isalpha() or ";" not in line:
            continue
        prefix, ffl = line.split(";", 1)
        out.append((prefix.strip(), ffl.strip()))
    return out
