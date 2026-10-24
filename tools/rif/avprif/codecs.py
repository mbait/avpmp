"""Typed codecs for RIF leaf chunks relevant to 3D models.

decode(chunk_id, data) -> dict, encode(chunk_id, dict) -> bytes.

Layouts follow the fill_data_block()/constructor pairs in src/win95/*.cpp; the
section in docs/RESOURCES.md documents each one with source references.
All values are little-endian; "pstr" is a NUL-terminated string padded with
1..4 NULs to a multiple of 4 bytes ((len + 4) & ~3), the CHUNK_*_STRING rule.
"""

import struct

from .chunks import norm_id, id_str


class Reader:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def i32(self, n=None):
        if n is None:
            v = struct.unpack_from("<i", self.d, self.p)[0]
            self.p += 4
            return v
        v = list(struct.unpack_from("<%di" % n, self.d, self.p))
        self.p += 4 * n
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.d, self.p)[0]
        self.p += 4
        return v

    def f32(self, n=None):
        if n is None:
            v = struct.unpack_from("<f", self.d, self.p)[0]
            self.p += 4
            return v
        v = list(struct.unpack_from("<%df" % n, self.d, self.p))
        self.p += 4 * n
        return v

    def f64(self):
        v = struct.unpack_from("<d", self.d, self.p)[0]
        self.p += 8
        return v

    def raw(self, n):
        v = self.d[self.p:self.p + n]
        self.p += n
        return bytes(v)

    def cstr(self):
        """NUL-terminated string, no padding."""
        e = self.d.index(b"\0", self.p)
        s = self.d[self.p:e].decode("latin-1")
        self.p = e + 1
        return s

    def pstr(self):
        """NUL-terminated string padded to (len+4)&~3."""
        e = self.d.index(b"\0", self.p)
        s = self.d[self.p:e].decode("latin-1")
        self.p += (e - self.p + 4) & ~3
        return s

    def fixed_str(self, n):
        s = self.d[self.p:self.p + n].split(b"\0")[0].decode("latin-1")
        self.p += n
        return s

    def rest(self):
        return len(self.d) - self.p


class Writer:
    """Byte builder that remembers which bytes are padding (for comparisons)."""

    def __init__(self):
        self.b = bytearray()
        self.pad = []

    def i32(self, *vals):
        for v in vals:
            self.b += struct.pack("<i", int(v))

    def u32(self, v):
        self.b += struct.pack("<I", int(v) & 0xFFFFFFFF)

    def f32(self, *vals):
        for v in vals:
            self.b += struct.pack("<f", float(v))

    def f64(self, v):
        self.b += struct.pack("<d", float(v))

    def raw(self, b):
        self.b += b

    def _padzero(self, n):
        start = len(self.b)
        self.b += b"\0" * n
        self.pad.append((start, start + n))

    def cstr(self, s):
        self.b += s.encode("latin-1") + b"\0"

    def pstr(self, s):
        e = s.encode("latin-1")
        self.b += e + b"\0"
        self._padzero(((len(e) + 4) & ~3) - len(e) - 1)

    def fixed_str(self, s, n):
        e = s.encode("latin-1")[:n]
        self.b += e
        self._padzero(n - len(e))

    def align4(self):
        self._padzero((-len(self.b)) % 4)

    def bytes(self):
        return bytes(self.b)


DECODERS = {}
ENCODERS = {}


def codec(*ids):
    def deco(cls):
        for i in ids:
            DECODERS[norm_id(i)] = cls.decode
            ENCODERS[norm_id(i)] = cls.encode
        return cls
    return deco


def has_codec(cid):
    return norm_id(cid) in DECODERS


def decode(cid, data):
    return DECODERS[norm_id(cid)](Reader(data))


def encode_w(cid, d):
    w = Writer()
    ENCODERS[norm_id(cid)](w, d)
    return w


def encode(cid, d):
    return encode_w(cid, d).bytes()


# ---------------------------------------------------------------- simple

@codec("RIFVERIN", "BMNAMVER", "OBASEQTM", "OBASEQFL", "CONSTYPE")
class OneInt:
    @staticmethod
    def decode(r):
        return {"value": r.i32()}

    @staticmethod
    def encode(w, d):
        w.i32(d["value"])


@codec("RIFFNAME", "OBHIERNM", "SHPFNAME", "SOUNDDIR")
class OneString:
    @staticmethod
    def decode(r):
        return {"name": r.pstr()}

    @staticmethod
    def encode(w, d):
        w.pstr(d["name"])


@codec("ENVSDSCL")
class EnvScale:
    @staticmethod
    def decode(r):
        return {"scale": r.f64()}

    @staticmethod
    def encode(w, d):
        w.f64(d["scale"])


@codec("ENDTHEAD")
class EnvHeader:
    @staticmethod
    def decode(r):
        return {"flags": r.i32(), "lock_user": r.fixed_str(16), "version_no": r.i32()}

    @staticmethod
    def encode(w, d):
        w.i32(d["flags"])
        w.fixed_str(d.get("lock_user", ""), 16)
        w.i32(d.get("version_no", 0))


@codec("BMNAMEXT")
class BmpNameExt:
    @staticmethod
    def decode(r):
        return {"flags": r.i32(), "reserved": r.i32(12)}

    @staticmethod
    def encode(w, d):
        w.i32(d["flags"])
        w.i32(*d.get("reserved", [0] * 12))


@codec("OBJNOTES")
class Notes:
    @staticmethod
    def decode(r):
        return {"text": r.raw(r.rest())}

    @staticmethod
    def encode(w, d):
        w.raw(d["text"])
        w.align4()


# ---------------------------------------------------------------- textures

@codec("BMPNAMES", "BMPLSTST")
class BmpNames:
    """Texture list. Entry index is what polygon colour & 0xfff refers to."""

    @staticmethod
    def decode(r):
        hdr = r.i32()
        n = hdr & 0xFFFF
        out = {"legacy_version": (hdr >> 16) & 0xFFFF, "entries": []}
        for _ in range(n):
            flags, index, verenum, prio, transp = r.i32(5)
            out["entries"].append({
                "flags": flags, "index": index,
                "version_num": verenum & 0xFFFFF, "enum_id": (verenum >> 20) & 0xFFF,
                "priority": prio, "transparency": transp & 0xFFFFFFFF,
                "filename": r.pstr()})
        return out

    @staticmethod
    def encode(w, d):
        es = d["entries"]
        w.i32((len(es) & 0xFFFF) | (d.get("legacy_version", 0) << 16))
        for e in es:
            w.i32(e["flags"], e["index"],
                  (e.get("version_num", 0) & 0xFFFFF) | (e.get("enum_id", 0) << 20),
                  e.get("priority", 0))
            w.u32(e.get("transparency", 0))
            w.pstr(e["filename"])


# ---------------------------------------------------------------- shapes

@codec("SHPHEAD1")
class ShapeHeader:
    @staticmethod
    def decode(r):
        d = {"flags": r.i32(), "lock_user": r.fixed_str(16), "file_id_num": r.i32(),
             "num_verts": r.i32(), "num_polys": r.i32(), "radius": r.f32()}
        mx, mnx, my, mny, mz, mnz = r.i32(6)
        d["max"] = [mx, my, mz]
        d["min"] = [mnx, mny, mnz]
        d["version_no"] = r.i32()
        n = r.i32()
        d["object_names"] = [r.cstr() for _ in range(n)]
        return d

    @staticmethod
    def encode(w, d):
        w.i32(d.get("flags", 0))
        w.fixed_str(d.get("lock_user", ""), 16)
        w.i32(d["file_id_num"], d["num_verts"], d["num_polys"])
        w.f32(d["radius"])
        mx, my, mz = d["max"]
        mnx, mny, mnz = d["min"]
        w.i32(mx, mnx, my, mny, mz, mnz)
        w.i32(d.get("version_no", 0))
        names = d.get("object_names", [])
        w.i32(len(names))
        for s in names:
            w.cstr(s)
        w.align4()


@codec("SUBSHPHD")
class SubShapeHeader:
    @staticmethod
    def decode(r):
        d = {"flags": r.i32(), "file_id_num": r.i32(), "num_verts": r.i32(),
             "num_polys": r.i32(), "radius": r.f32()}
        mx, mnx, my, mny, mz, mnz = r.i32(6)
        d["max"] = [mx, my, mz]
        d["min"] = [mnx, mny, mnz]
        return d

    @staticmethod
    def encode(w, d):
        w.i32(d.get("flags", 0), d["file_id_num"], d["num_verts"], d["num_polys"])
        w.f32(d["radius"])
        mx, my, mz = d["max"]
        mnx, mny, mnz = d["min"]
        w.i32(mx, mnx, my, mny, mz, mnz)


@codec("SHPRAWVT")
class RawVerts:
    @staticmethod
    def decode(r):
        n = r.rest() // 12
        v = r.i32(3 * n)
        return {"verts": [v[i:i + 3] for i in range(0, 3 * n, 3)]}

    @staticmethod
    def encode(w, d):
        for v in d["verts"]:
            w.i32(*v)


@codec("SHPVNORM", "SHPPNORM")
class Normals:
    @staticmethod
    def decode(r):
        n = r.rest() // 12
        v = r.f32(3 * n)
        return {"normals": [v[i:i + 3] for i in range(0, 3 * n, 3)]}

    @staticmethod
    def encode(w, d):
        for v in d["normals"]:
            w.f32(*v)


@codec("SHPPOLYS")
class Polys:
    """36-byte records: engine_type, normal_index, flags, colour, vert[5]."""

    @staticmethod
    def decode(r):
        n = r.rest() // 36
        out = []
        for _ in range(n):
            et, ni, fl = r.i32(3)
            col = r.u32()
            vs = r.i32(5)
            verts = []
            for v in vs:
                if v == -1:
                    break
                verts.append(v)
            out.append({"engine_type": et, "normal_index": ni, "flags": fl & 0xFFFFFFFF,
                        "colour": col, "verts": verts})
        return {"polys": out}

    @staticmethod
    def encode(w, d):
        for p in d["polys"]:
            vs = list(p["verts"])
            if not 3 <= len(vs) <= 4:
                raise ValueError("polygon must have 3 or 4 vertices")
            w.i32(p["engine_type"], p["normal_index"])
            w.u32(p["flags"])
            w.u32(p["colour"])
            w.i32(*(vs + [-1] * (5 - len(vs))))


@codec("SHPUVCRD")
class UVs:
    @staticmethod
    def decode(r):
        n = r.i32()
        out = []
        for _ in range(n):
            nv = r.i32()
            f = r.f32(2 * nv)
            out.append([f[i:i + 2] for i in range(0, 2 * nv, 2)])
        return {"uv_lists": out}

    @staticmethod
    def encode(w, d):
        w.i32(len(d["uv_lists"]))
        for lst in d["uv_lists"]:
            w.i32(len(lst))
            for u, v in lst:
                w.f32(u, v)


@codec("SHPTEXFN")
class TexFilenames:
    @staticmethod
    def decode(r):
        n = r.i32()
        return {"names": [r.cstr() for _ in range(n)]}

    @staticmethod
    def encode(w, d):
        w.i32(len(d["names"]))
        for s in d["names"]:
            w.cstr(s)
        w.align4()


@codec("SHPCENTR")
class ShapeCentre:
    @staticmethod
    def decode(r):
        return {"centre": r.i32(3), "radius": r.f32()}

    @staticmethod
    def encode(w, d):
        w.i32(*d["centre"])
        w.f32(d["radius"])


@codec("SHPMRGDT")
class MergeData:
    @staticmethod
    def decode(r):
        return {"merge": r.i32(r.rest() // 4)}

    @staticmethod
    def encode(w, d):
        w.i32(*d["merge"])


@codec("SHPPCINF")
class PolyChangeInfo:
    @staticmethod
    def decode(r):
        orig = r.i32()
        n = r.i32()
        return {"original_num_verts": orig,
                "changes": [r.i32(3) for _ in range(n)]}

    @staticmethod
    def encode(w, d):
        w.i32(d["original_num_verts"], len(d["changes"]))
        for c in d["changes"]:
            w.i32(*c)


@codec("SHPEXTFN")
class ExtFilename:
    @staticmethod
    def decode(r):
        return {"rescale": r.f64(), "version_no": r.i32(), "filename": r.pstr()}

    @staticmethod
    def encode(w, d):
        w.f64(d["rescale"])
        w.i32(d["version_no"])
        w.pstr(d["filename"])


@codec("EXTOBJNM")
class ExtObjName:
    @staticmethod
    def decode(r):
        return {"pad": r.i32(), "name": r.pstr()}

    @staticmethod
    def encode(w, d):
        w.i32(d.get("pad", 0))
        w.pstr(d["name"])


@codec("FRMMORPH")
class FrameMorph:
    @staticmethod
    def decode(r):
        fl, sp, n = r.i32(3)
        return {"flags": fl, "speed": sp, "frames": [r.i32(3) for _ in range(n)]}

    @staticmethod
    def encode(w, d):
        w.i32(d["flags"], d["speed"], len(d["frames"]))
        for f in d["frames"]:
            w.i32(*f)


@codec("ANISEQDT", "ANIFRADT")
class AnimSeqData:
    """ANISEQDT: SequenceNum, flags, pad2..4, name;
    ANIFRADT: FrameNum, flags, num_interp_frames, pad3, pad4, name.
    Name is padded with (len + 6) & ~3 bytes (shpchunk.cpp size formula)."""

    @staticmethod
    def decode(r):
        vals = r.i32(5)
        e = r.d.index(b"\0", r.p)
        name = r.d[r.p:e].decode("latin-1")
        r.p += (len(name) + 6) & ~3
        return {"values": vals, "name": name}

    @staticmethod
    def encode(w, d):
        w.i32(*d["values"])
        e = d["name"].encode("latin-1")
        w.raw(e + b"\0")
        w._padzero(((len(e) + 6) & ~3) - len(e) - 1)


@codec("ANSHCEN2")
class AnimShapeCentre:
    @staticmethod
    def decode(r):
        return {"centre": r.i32(3), "flags": r.i32(), "pad": r.i32()}

    @staticmethod
    def encode(w, d):
        w.i32(*d["centre"])
        w.i32(d.get("flags", 0), d.get("pad", 0))


@codec("TEXTANIM")
class TextureAnim:
    @staticmethod
    def decode(r):
        polys = []
        for _ in range(r.i32()):
            poly, pid, nseq, nverts, aflags, ident = r.i32(6)
            seqs = []
            for _ in range(nseq):
                speed, flags, nframes, s1, s2 = r.i32(5)
                tex = r.i32(nframes)
                uv = r.i32(2 * nverts * nframes)
                seqs.append({"speed": speed, "flags": flags & 0xFFFFFFFF, "spare": [s1, s2],
                             "textures": tex, "uvs": uv})
            polys.append({"poly": poly, "id": pid, "num_verts": nverts,
                          "anim_flags": aflags, "identifier": ident, "sequences": seqs})
        return {"polys": polys}

    @staticmethod
    def encode(w, d):
        w.i32(len(d["polys"]))
        for p in d["polys"]:
            w.i32(p["poly"], p["id"], len(p["sequences"]), p["num_verts"],
                  p["anim_flags"], p["identifier"])
            for s in p["sequences"]:
                w.i32(s["speed"])
                w.u32(s["flags"])
                w.i32(len(s["textures"]), *s.get("spare", [0, 0]))
                w.i32(*s["textures"])
                w.i32(*s["uvs"])


# ---------------------------------------------------------------- objects

@codec("OBJHEAD1")
class ObjectHeader:
    @staticmethod
    def decode(r):
        return {"flags": r.i32(), "lock_user": r.fixed_str(16), "location": r.i32(3),
                "orientation": r.f32(4), "index_num": r.i32(), "version_no": r.i32(),
                "shape_id_no": r.i32(), "name": r.pstr()}

    @staticmethod
    def encode(w, d):
        w.i32(d.get("flags", 0))
        w.fixed_str(d.get("lock_user", ""), 16)
        w.i32(*d.get("location", [0, 0, 0]))
        w.f32(*d.get("orientation", [0, 0, 0, 1]))
        w.i32(d.get("index_num", 0), d.get("version_no", 0), d["shape_id_no"])
        # names > 49 chars overflow the engine's char[50] (validate.py checks)
        w.pstr(d["name"])


@codec("SHPVTINT")
class VertexIntensities:
    @staticmethod
    def decode(r):
        name = r.fixed_str(8)
        pad = r.i32()
        n = r.i32()
        return {"light_set": name, "pad": pad, "intensities": r.i32(n)}

    @staticmethod
    def encode(w, d):
        w.fixed_str(d["light_set"], 8)
        w.i32(d.get("pad", 0), len(d["intensities"]), *d["intensities"])


# ---------------------------------------------------------------- hierarchy

@codec("OBJHIERD")
class HierarchyData:
    @staticmethod
    def decode(r):
        n = r.i32()
        return {"extra": r.i32(n), "object_name": r.pstr()}

    @staticmethod
    def encode(w, d):
        ex = d.get("extra", [])
        w.i32(len(ex), *ex)
        w.pstr(d["object_name"])


@codec("HIERBBOX")
class HierBBox:
    @staticmethod
    def decode(r):
        return {"min": r.i32(3), "max": r.i32(3)}

    @staticmethod
    def encode(w, d):
        w.i32(*d["min"])
        w.i32(*d["max"])


@codec("OBASEQHD")
class AnimSeqHeader:
    @staticmethod
    def decode(r):
        nf = r.u32()
        seq, sub, n = r.i32(3)
        return {"num_frames": nf, "sequence": seq, "sub_sequence": sub,
                "extra": r.i32(n), "name": r.pstr()}

    @staticmethod
    def encode(w, d):
        w.u32(d.get("num_frames", 65536))
        ex = d.get("extra", [])
        w.i32(d["sequence"], d["sub_sequence"], len(ex), *ex)
        w.pstr(d.get("name", ""))


@codec("OBASEQFR")
class AnimSeqFrame:
    @staticmethod
    def decode(r):
        q = r.f32(4)
        t = r.i32(3)
        at, ref = r.i32(2)
        fl = r.u32()
        n = r.i32()
        return {"quat": q, "offset": t, "at_frame_no": at, "frame_ref_no": ref,
                "flags": fl, "extra": r.i32(n)}

    @staticmethod
    def encode(w, d):
        w.f32(*d["quat"])
        w.i32(*d["offset"])
        w.i32(d["at_frame_no"], d["frame_ref_no"])
        w.u32(d.get("flags", 0))
        ex = d.get("extra", [])
        w.i32(len(ex), *ex)


@codec("OBASEQSP")
class AnimSeqSpeed:
    @staticmethod
    def decode(r):
        return {"speed": r.i32(), "angle": r.i32(), "spare": r.i32()}

    @staticmethod
    def encode(w, d):
        w.i32(d["speed"], d["angle"], d.get("spare", 0))


@codec("OBANALLS")
class AnimAll:
    """All sequences of one section; frames: quat(4f) offset(3i) at_frame_no flags."""

    @staticmethod
    def decode(r):
        seqs = []
        for _ in range(r.i32()):
            nf = r.u32()
            seq, sub = r.i32(2)
            tm = r.u32()
            frames = []
            for _ in range(nf):
                q = r.f32(4)
                t = r.i32(3)
                at = r.i32()
                fl = r.u32()
                frames.append({"quat": q, "offset": t, "at_frame_no": at, "flags": fl})
            seqs.append({"sequence": seq, "sub_sequence": sub, "time_ms": tm,
                         "frames": frames})
        return {"sequences": seqs}

    @staticmethod
    def encode(w, d):
        w.i32(len(d["sequences"]))
        for s in d["sequences"]:
            w.u32(len(s["frames"]))
            w.i32(s["sequence"], s["sub_sequence"])
            w.u32(s.get("time_ms", 0))
            for f in s["frames"]:
                w.f32(*f["quat"])
                w.i32(*f["offset"])
                w.i32(f["at_frame_no"])
                w.u32(f.get("flags", 0))


@codec("OBHALTSH")
class AltShapeSet:
    @staticmethod
    def decode(r):
        num = r.i32()
        name = r.pstr()
        n = r.i32()
        rep = [[r.pstr(), r.pstr()] for _ in range(n)]
        flags = r.i32()
        spare = r.i32(3)
        return {"num": num, "name": name, "replacements": rep, "flags": flags,
                "spare": spare}

    @staticmethod
    def encode(w, d):
        w.i32(d["num"])
        w.pstr(d["name"])
        w.i32(len(d["replacements"]))
        for a, b in d["replacements"]:
            w.pstr(a)
            w.pstr(b)
        w.i32(d.get("flags", 0), *d.get("spare", [0, 0, 0]))


@codec("HSETCOLL")
class ShapeSetCollection:
    @staticmethod
    def decode(r):
        num = r.i32()
        name = r.pstr()
        n = r.i32()
        return {"num": num, "name": name, "sets": r.i32(n), "type_index": r.i32(),
                "flags": r.i32()}

    @staticmethod
    def encode(w, d):
        w.i32(d["num"])
        w.pstr(d["name"])
        w.i32(len(d["sets"]), *d["sets"])
        w.i32(d.get("type_index", 0), d.get("flags", 0))


@codec("HIDEGDIS")
class LodDistances:
    @staticmethod
    def decode(r):
        n = r.i32()
        return {"distances": r.i32(n)}

    @staticmethod
    def encode(w, d):
        w.i32(len(d["distances"]), *d["distances"])


@codec("INDSOUND")
class IndexedSound:
    @staticmethod
    def decode(r):
        d = {"index": r.i32(), "wav": r.pstr()}
        d["inner_range"], d["outer_range"], d["max_volume"], d["pitch"], d["flags"] = r.i32(5)
        n = r.i32()
        d["extra"] = r.i32(n)
        return d

    @staticmethod
    def encode(w, d):
        w.i32(d["index"])
        w.pstr(d["wav"])
        w.i32(d["inner_range"], d["outer_range"], d["max_volume"], d["pitch"],
              d.get("flags", 0))
        ex = d.get("extra", [])
        w.i32(len(ex), *ex)


# ---------------------------------------------------------------- helpers

def decode_chunk(ch):
    return decode(ch.id, ch.data)


def set_chunk(ch, d):
    ch.data = encode(ch.id, d)


def roundtrip_ok(ch):
    """Re-encode a decoded chunk and compare with the original, ignoring the
    padding bytes (the original tools left them uninitialised)."""
    d = decode(ch.id, ch.data)
    w = encode_w(ch.id, d)
    a, b = ch.data, w.bytes()
    if len(a) != len(b):
        return False, "length %d != %d" % (len(a), len(b))
    pad = bytearray(len(b))
    for s, e in w.pad:
        for i in range(s, e):
            pad[i] = 1
    for i in range(len(a)):
        if a[i] != b[i] and not pad[i]:
            return False, "byte %d differs (%02x vs %02x) in %s" % (i, a[i], b[i], id_str(ch.id))
    return True, ""
