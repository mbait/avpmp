#!/usr/bin/env python3
"""Install AvP Gold Edition data from the retail CD for the Linux port.

Replicates what the Windows InstallShield setup does:
  * copies avp_huds/, avp_rifs/, fastfile/ (textures), a sound set,
    English/ (language.txt etc.), "CD Tracks.txt", FMVs/ and MPConfig/
  * descrambles the files that are stored scrambled on the CD
    (see avprif/disc.py and docs/RESOURCES.md, "CD scrambling")
  * lowercases every file and directory name (required by the port)

Usage:
    avp_install.py SOURCE DEST [--sound high|low] [--no-fmv]

SOURCE is either the root of the mounted/extracted CD, or a raw CloneCD /
BIN image (.img/.bin, 2352-byte sectors) or a plain .iso, which is read
directly with the built-in ISO9660 reader.
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "rif"))
from avprif import disc  # noqa: E402


# ------------------------------------------------------------ image access

class RawImage:
    """Sector access for .iso (2048) or raw .img/.bin (2352) images."""

    def __init__(self, path):
        self.f = open(path, "rb")
        size = os.path.getsize(path)
        self.raw = size % 2048 != 0 or self._looks_raw()
        self.ssize = 2352 if self.raw else 2048

    def _looks_raw(self):
        self.f.seek(0)
        return self.f.read(12) == b"\x00" + b"\xff" * 10 + b"\x00"

    def sector(self, lba):
        self.f.seek(lba * self.ssize)
        s = self.f.read(self.ssize)
        if not self.raw:
            return s
        off = 16 if s[15] == 1 else 24  # Mode 1 / Mode 2 Form 1
        return s[off:off + 2048]

    def read(self, lba, size):
        out = bytearray()
        while len(out) < size:
            out += self.sector(lba)
            lba += 1
        return bytes(out[:size])


class IsoFS:
    """Minimal ISO9660 reader; prefers the Joliet tree for long names."""

    def __init__(self, img):
        self.img = img
        self.joliet = False
        pvd = img.sector(16)
        if pvd[1:6] != b"CD001":
            raise ValueError("no ISO9660 primary volume descriptor")
        self.root = self._rec(pvd[156:190])
        lba = 17
        while True:
            vd = img.sector(lba)
            if vd[1:6] != b"CD001" or vd[0] == 255:
                break
            if vd[0] == 2 and vd[88:91] in (b"%/@", b"%/C", b"%/E"):
                self.joliet = True
                self.root = self._rec(vd[156:190])
                break
            lba += 1

    @staticmethod
    def _rec(r):
        lba, size = struct.unpack_from("<I", r, 2)[0], struct.unpack_from("<I", r, 10)[0]
        flags = r[25]
        name = r[33:33 + r[32]]
        return name, lba, size, flags

    def _name(self, raw):
        if self.joliet and raw not in (b"\0", b"\1"):
            return raw.decode("utf-16-be")
        return raw.decode("latin-1")

    def listdir(self, entry):
        _, lba, size, _ = entry
        data = self.img.read(lba, size)
        i = 0
        while i < len(data):
            ln = data[i]
            if ln == 0:
                i = (i // 2048 + 1) * 2048
                continue
            name, l, s, fl = self._rec(data[i:i + ln])
            i += ln
            if name in (b"\0", b"\1"):
                continue
            name = self._name(name).split(";")[0]
            if name.endswith("."):
                name = name[:-1]
            yield name, (name, l, s, fl)

    def walk(self, entry=None, prefix=""):
        for name, e in self.listdir(entry or self.root):
            path = prefix + name
            if e[3] & 2:
                yield from self.walk(e, path + "/")
            else:
                yield path, e

    def readfile(self, e):
        return self.img.read(e[1], e[2])


class DirFS:
    def __init__(self, root):
        self.root = root

    def walk(self):
        for r, _, fs in os.walk(self.root):
            for f in fs:
                p = os.path.join(r, f)
                yield os.path.relpath(p, self.root).replace(os.sep, "/"), p

    def readfile(self, p):
        with open(p, "rb") as f:
            return f.read()


# ------------------------------------------------------------- mapping

def plan(path, sound, fmv):
    """Map a CD path to its installed path (or None to skip)."""
    low = path.lower()
    parts = low.split("/")
    top = parts[0]
    if top in ("avp_huds", "avp_rifs"):
        return low
    if top == "fastfile":
        if len(parts) != 2:  # fastfile/1/ holds stale duplicates
            return None
        return low
    if top == "english":
        return parts[-1]
    if top == ("hsound" if sound == "high" else "lsound") or top == "rsound":
        return "fastfile/" + parts[-1]
    if top == "fmvs":
        return low if fmv else None
    if top == "mpconfig":
        return low
    if low == "cd tracks.txt":
        return low
    return None


SCRAMBLED_DIRS = ("avp_huds/", "avp_rifs/", "fastfile/")


def needs_descramble(out_path, data):
    return out_path.startswith(SCRAMBLED_DIRS) and disc.is_scrambled(data[:64])


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source")
    ap.add_argument("dest")
    ap.add_argument("--sound", choices=("high", "low"), default="high")
    ap.add_argument("--no-fmv", action="store_true")
    ap.add_argument("-n", "--dry-run", action="store_true")
    a = ap.parse_args()

    if os.path.isdir(a.source):
        fs = DirFS(a.source)
    else:
        fs = IsoFS(RawImage(a.source))

    n = nd = 0
    # rsound must override hsound/lsound duplicates, so sort it last
    entries = sorted(fs.walk(), key=lambda x: x[0].lower().startswith("rsound"))
    for path, h in entries:
        out = plan(path, a.sound, not a.no_fmv)
        if out is None:
            continue
        data = fs.readfile(h)
        scr = needs_descramble(out, data)
        if scr:
            data = disc.decode(data)
            nd += 1
        n += 1
        print("%-12s %s -> %s" % ("descramble" if scr else "copy", path, out))
        if a.dry_run:
            continue
        dst = os.path.join(a.dest, out)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        with open(dst, "wb") as f:
            f.write(data)
    print("%d files installed, %d descrambled" % (n, nd))


if __name__ == "__main__":
    main()
