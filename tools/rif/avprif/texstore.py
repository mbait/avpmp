"""Resolve texture names the way the game does (src/win95/chnktexi.cpp):
fastfile archives first ("Graphics\\" + name, then "graphics/" + name), then
loose files graphics/<name> under the data directory (case-insensitive)."""

import os

from . import ffl, image


class TextureStore:
    def __init__(self, data_dir):
        self.data_dir = data_dir
        self._ff = None
        self._disk = None

    def _index_ffl(self):
        self._ff = {}
        ffdir = os.path.join(self.data_dir, "fastfile")
        if not os.path.isdir(ffdir):
            return
        for f in sorted(os.listdir(ffdir)):
            if not f.lower().startswith("tex") or not f.lower().endswith(".ffl"):
                continue
            path = os.path.join(ffdir, f)
            with open(path, "rb") as fh:
                for name, blob in ffl.read(fh.read()):
                    key = name.lower().replace("/", "\\")
                    self._ff.setdefault(key, blob)

    def _index_disk(self):
        self._disk = {}
        g = os.path.join(self.data_dir, "graphics")
        for r, _, fs in os.walk(g):
            for f in fs:
                p = os.path.join(r, f)
                rel = os.path.relpath(p, g).replace(os.sep, "\\").lower()
                self._disk[rel] = p

    def find(self, name):
        """name: BMPNAMES filename, e.g. 'NPCs\\Alien\\Bits.RIM' -> bytes or None."""
        key = name.lower().replace("/", "\\")
        if self._ff is None:
            self._index_ffl()
        b = self._ff.get("graphics\\" + key)
        if b is not None:
            return b
        if self._disk is None:
            self._index_disk()
        p = self._disk.get(key)
        if p:
            with open(p, "rb") as f:
                return f.read()
        return None

    def in_fastfile(self, name):
        if self._ff is None:
            self._index_ffl()
        return ("graphics\\" + name.lower().replace("/", "\\")) in self._ff


def decode_any(data):
    """-> (w, h, rgba top-down) for ILBM/BMP data."""
    if data[:4] in (b"FORM", b"LIST", b"CAT "):
        return image.decode_ilbm(data)
    if data[:2] == b"BM":
        return image.read_bmp(data)
    raise ValueError("unsupported texture format %r" % data[:4])
