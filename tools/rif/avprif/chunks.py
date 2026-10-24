"""Generic RIF chunk tree reader/writer.

Every chunk is   char id[8] | int32 size (incl. this 12-byte header) | body.
Container chunks (Chunk_With_Children in src/win95/chunk.hpp) have a body that
is just a concatenation of child chunks; all others are leaves whose body is
interpreted by a typed codec (see codecs.py) or kept as opaque bytes.

The set of containers mirrors the RIF_IMPLEMENT_DYNCREATE registrations in
the engine source: (id -> parent ids it is recognised under, None = any).
"""

import os
import struct

from . import huffman, disc

ROOT_ID = b"REBINFF2"

CONTAINERS = {
    b"REBINFF2": None,
    b"REBSHAPE": None,
    b"SUBSHAPE": None,
    b"SHPMORPH": {b"REBSHAPE", b"SUBSHAPE"},
    b"SHPEXTFL": None,
    b"SHPFRAGS": None,
    b"ANIMSEQU": None,
    b"ANIMFRAM": {b"ANIMSEQU"},
    b"ASALTTEX": None,
    b"RBOBJECT": None,
    b"OBINTDT\0": None,
    b"MODULEDT": None,
    b"OBJPRJDT": None,
    b"OBJCHIER": None,
    b"OBANSEQS": None,
    b"OBANSEQC": None,
    b"REBENVDT": None,
    b"GAMEMODE": None,
    b"LIGHTSET": None,
    b"SPECLOBJ": None,
    b"FRAGTYPE": None,
    b"REBENUMS": None,
    b"DUMMYOBJ": None,
    b"RSPRITES": None,
    b"SPRIHEAD": None,
    b"SPRITEPC": {b"SPRIHEAD"},
    b"SPRITEPS": {b"SPRIHEAD"},
    b"SPRITESA": {b"SPRIHEAD"},
    b"AVPGENEX": None,
    b"SOUNDEXD": None,
    b"PLACHIER": None,
    b"VIOBJECT": None,
}


def norm_id(cid):
    if isinstance(cid, str):
        cid = cid.encode("ascii")
    return cid.ljust(8, b"\0")[:8]


def id_str(cid):
    return cid.rstrip(b"\0").decode("latin-1")


class Chunk:
    """A node of the chunk tree.

    Leaves keep their raw body in .data; containers keep .children.
    """

    __slots__ = ("id", "data", "children", "parent")

    def __init__(self, cid, data=b"", children=None):
        self.id = norm_id(cid)
        self.data = data
        self.children = children
        self.parent = None
        if children is not None:
            for c in children:
                c.parent = self

    # -- tree helpers
    @property
    def is_container(self):
        return self.children is not None

    @property
    def name(self):
        return id_str(self.id)

    def add(self, child, index=None):
        child.parent = self
        if index is None:
            self.children.append(child)
        else:
            self.children.insert(index, child)
        return child

    def remove(self, child):
        self.children.remove(child)
        child.parent = None

    def find(self, cid):
        cid = norm_id(cid)
        for c in self.children or ():
            if c.id == cid:
                return c
        return None

    def findall(self, cid):
        cid = norm_id(cid)
        return [c for c in self.children or () if c.id == cid]

    def walk(self, depth=0):
        yield depth, self
        for c in self.children or ():
            yield from c.walk(depth + 1)

    # -- (de)serialisation
    def size(self):
        if self.children is None:
            return 12 + len(self.data)
        return 12 + sum(c.size() for c in self.children)

    def serialize(self):
        out = bytearray()
        self._write(out)
        return bytes(out)

    def _write(self, out):
        out += self.id + struct.pack("<i", self.size())
        if self.children is None:
            out += self.data
        else:
            for c in self.children:
                c._write(out)

    def __repr__(self):
        if self.children is None:
            return "<%s %d bytes>" % (self.name, len(self.data))
        return "<%s [%d children]>" % (self.name, len(self.children))


class RifError(Exception):
    pass


def _is_container(cid, parent_id):
    if cid not in CONTAINERS:
        return False
    parents = CONTAINERS[cid]
    return parents is None or parent_id in parents


def parse_children(buf, start, end, parent_id, strict=True):
    """Parse the chunks in buf[start:end] (mirrors CHUNK_WITH_CHILDREN_LOADER)."""
    kids = []
    pos = start
    while pos < end:
        if end - pos < 12:
            if strict:
                raise RifError("truncated chunk header at %d" % pos)
            break
        cid = bytes(buf[pos:pos + 8])
        size = struct.unpack_from("<i", buf, pos + 8)[0]
        if size < 12 or pos + size > end:
            raise RifError("chunk %r at %d has bad size %d" % (cid, pos, size))
        if _is_container(cid, parent_id):
            ch = Chunk(cid, children=parse_children(buf, pos + 12, pos + size, cid, strict))
        else:
            ch = Chunk(cid, bytes(buf[pos + 12:pos + size]))
        kids.append(ch)
        pos += size
    return kids


def load_bytes(data):
    """Parse a RIF image (REBINFF2, REBCRIF1, or CD-scrambled either)."""
    if disc.is_scrambled(data[:64]):
        data = disc.decode(data)
    if data[:8] == huffman.IDENT:
        data = huffman.decompress(data)
    if data[:8] != ROOT_ID:
        raise RifError("not a RIF file (magic %r)" % data[:8])
    size = struct.unpack_from("<i", data, 8)[0]
    if size != len(data):
        raise RifError("REBINFF2 size field %d != data size %d" % (size, len(data)))
    return Chunk(ROOT_ID, children=parse_children(data, 12, size, ROOT_ID))


def load(path):
    with open(path, "rb") as f:
        return load_bytes(f.read())


def save(root, path, compress=False):
    data = root.serialize()
    if compress:
        data = huffman.compress(data)
    tmp = path + ".tmp"
    with open(tmp, "wb") as f:
        f.write(data)
    os.replace(tmp, path)
