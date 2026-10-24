"""High-level model API on top of the chunk tree.

Coordinates are AvP engine space: integer millimetres, x right, y DOWN,
z forward (a proper right-handed basis). Polygon front faces wind so that
normalize((v1-v0) x (v2-v0)) is the stored polygon normal.

Quaternions are (x, y, z, w) as stored in the files: a section's world
rotation is parent_world_rotation * R(q) (R = standard Hamilton matrix), and
its pivot sits at parent_pivot + parent_world_rotation * offset.
"""

import math

from . import codecs
from .chunks import Chunk

# polygon engine types (include/shape.h ShapeItems)
I_POLYGON = 2
I_GOURAUD_POLYGON = 3
I_3D_TEXTURED_POLYGON = 7
I_GOURAUD_3D_TEXTURED_POLYGON = 24

# polygon flags (include/shape.h iflag_*)
IFLAG_NOTVIS = 0x1
IFLAG_NOLIGHT = 0x2
IFLAG_TRANSPARENT = 0x10
IFLAG_NO_BFC = 0x20
IFLAG_LINEAR = 0x20000
IFLAG_TX2DOR3D = 0x400000
IFLAG_LINEAR_S = 0x800000

# BMPNAMES entry flags (win95/bmpnames.hpp)
BMPN_USES_TRANSPARENCY = 0x2
BMPN_PRIORITY_VALID = 0x100
BMPN_IFF = 0x1000000

# sequence time unit: keyframe positions are 0..65535 within a sequence
ONE_FIXED = 65536


# ---------------------------------------------------------------- vectors

def _sub(a, b):
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]


def _cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def _norm(v):
    ln = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    return [v[0] / ln, v[1] / ln, v[2] / ln] if ln > 1e-12 else [0.0, 0.0, 0.0]


def quat_to_mat(q):
    x, y, z, w = q
    return [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]


def mat_to_quat(m):
    t = m[0][0] + m[1][1] + m[2][2]
    if t > 0:
        s = math.sqrt(t + 1.0) * 2
        w = 0.25 * s
        x = (m[2][1] - m[1][2]) / s
        y = (m[0][2] - m[2][0]) / s
        z = (m[1][0] - m[0][1]) / s
    elif m[0][0] > m[1][1] and m[0][0] > m[2][2]:
        s = math.sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2
        w = (m[2][1] - m[1][2]) / s
        x = 0.25 * s
        y = (m[0][1] + m[1][0]) / s
        z = (m[0][2] + m[2][0]) / s
    elif m[1][1] > m[2][2]:
        s = math.sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2
        w = (m[0][2] - m[2][0]) / s
        x = (m[0][1] + m[1][0]) / s
        y = 0.25 * s
        z = (m[1][2] + m[2][1]) / s
    else:
        s = math.sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2
        w = (m[1][0] - m[0][1]) / s
        x = (m[0][2] + m[2][0]) / s
        y = (m[1][2] + m[2][1]) / s
        z = 0.25 * s
    n = math.sqrt(x * x + y * y + z * z + w * w)
    return [x / n, y / n, z / n, w / n]


# ---------------------------------------------------------------- shapes

class Polygon:
    __slots__ = ("verts", "uvs", "texture", "engine_type", "flags")

    def __init__(self, verts, uvs=None, texture=None, engine_type=None, flags=None):
        self.verts = list(verts)
        self.uvs = uvs            # list of (u, v) in TEXELS, or None
        self.texture = texture    # BMPNAMES index, or None for untextured
        if engine_type is None:
            engine_type = I_3D_TEXTURED_POLYGON if texture is not None else I_POLYGON
        self.engine_type = engine_type
        if flags is None:
            flags = IFLAG_TX2DOR3D | IFLAG_LINEAR_S if texture is not None else 0
        self.flags = flags


class Mesh:
    """One REBSHAPE: integer vertices + triangles/quads."""

    def __init__(self, verts=None, polys=None, colour=None):
        self.verts = [list(map(int, v)) for v in verts or []]
        self.polys = list(polys or [])
        self.colour = colour  # untextured polygon colour word (if any)

    def poly_normals(self):
        out = []
        for p in self.polys:
            a, b, c = (self.verts[i] for i in p.verts[:3])
            out.append(_norm(_cross(_sub(b, a), _sub(c, a))))
        return out

    def vertex_normals(self, pnorms=None):
        pnorms = pnorms or self.poly_normals()
        acc = [[0.0, 0.0, 0.0] for _ in self.verts]
        for p, n in zip(self.polys, pnorms):
            for i in p.verts:
                acc[i] = [acc[i][k] + n[k] for k in range(3)]
        return [_norm(v) if any(v) else [0.0, -1.0, 0.0] for v in acc]

    def bounds(self):
        if not self.verts:
            return [0, 0, 0], [0, 0, 0]
        mn = [min(v[k] for v in self.verts) for k in range(3)]
        mx = [max(v[k] for v in self.verts) for k in range(3)]
        return mn, mx

    def radius(self):
        return max((math.sqrt(v[0] ** 2 + v[1] ** 2 + v[2] ** 2) for v in self.verts), default=0.0)


def read_shape(shape_chunk):
    """REBSHAPE -> (Mesh, header dict)."""
    hd = codecs.decode_chunk(shape_chunk.find("SHPHEAD1"))
    verts = codecs.decode_chunk(shape_chunk.find("SHPRAWVT"))["verts"]
    polys = codecs.decode_chunk(shape_chunk.find("SHPPOLYS"))["polys"]
    uvc = shape_chunk.find("SHPUVCRD")
    uvl = codecs.decode_chunk(uvc)["uv_lists"] if uvc else []
    m = Mesh(verts)
    for p in polys:
        textured = p["engine_type"] in (5, 6, 7, 21, 22, 23, 24, 25)
        uv = None
        tex = None
        if textured:
            tex = p["colour"] & 0xFFF
            ui = p["colour"] >> 16
            if ui < len(uvl) and len(uvl[ui]) == len(p["verts"]):
                uv = [tuple(x) for x in uvl[ui]]
        poly = Polygon(p["verts"], uv, tex, p["engine_type"], p["flags"])
        if not textured:
            m.colour = p["colour"]
        m.polys.append(poly)
    return m, hd


def build_shape(mesh, file_id, object_names=(), centre_chunk=True):
    """Mesh -> REBSHAPE chunk with every child the loader dereferences."""
    pn = mesh.poly_normals()
    vn = mesh.vertex_normals(pn)
    mn, mx = mesh.bounds()
    uv_lists = []
    polys = []
    for i, p in enumerate(mesh.polys):
        if p.texture is not None:
            if not p.uvs or len(p.uvs) != len(p.verts):
                raise ValueError("textured polygon %d needs one UV per vertex" % i)
            colour = (len(uv_lists) << 16) | (p.texture & 0xFFF)
            uv_lists.append([list(map(float, uv)) for uv in p.uvs])
        else:
            colour = mesh.colour if mesh.colour is not None else 0xFFFFFFFF
            colour &= 0xFFFFFFFF
        polys.append({"engine_type": p.engine_type, "normal_index": i, "flags": p.flags,
                      "colour": colour, "verts": p.verts})
    sh = Chunk("REBSHAPE", children=[])

    def leaf(cid, d):
        sh.add(Chunk(cid, codecs.encode(cid, d)))

    leaf("SHPHEAD1", {"flags": 0, "file_id_num": file_id, "num_verts": len(mesh.verts),
                      "num_polys": len(mesh.polys), "radius": mesh.radius(),
                      "max": mx, "min": mn, "version_no": 1,
                      "object_names": list(object_names)})
    leaf("SHPRAWVT", {"verts": mesh.verts})
    leaf("SHPVNORM", {"normals": vn})
    leaf("SHPPNORM", {"normals": pn})
    leaf("SHPPOLYS", {"polys": polys})
    leaf("SHPUVCRD", {"uv_lists": uv_lists})
    if centre_chunk:
        c = [(mn[k] + mx[k]) // 2 for k in range(3)]
        r = max((math.sqrt(sum((v[k] - c[k]) ** 2 for k in range(3))) for v in mesh.verts),
                default=0.0)
        leaf("SHPCENTR", {"centre": c, "radius": r})
    return sh


def build_object(name, shape_id, location=(0, 0, 0), orientation=(0, 0, 0, 1),
                 flags=0, index=0):
    ob = Chunk("RBOBJECT", children=[])
    ob.add(Chunk("OBJHEAD1", codecs.encode("OBJHEAD1", {
        "flags": flags, "location": list(location), "orientation": list(orientation),
        "index_num": index, "version_no": 1, "shape_id_no": shape_id, "name": name})))
    return ob


# ---------------------------------------------------------------- textures

def texture_entry(index, filename, transparent=False):
    """BMPNAMES entry for a loose texture under graphics/ (IFF/relative path).

    filename uses backslashes, relative to graphics/, e.g. "open\\crate.bmp".
    The loader resolves it via fastfiles first, then graphics/<filename> on
    disk; pick names that do not collide with stock fastfile contents.
    """
    flags = BMPN_IFF | BMPN_PRIORITY_VALID
    if transparent:
        flags |= BMPN_USES_TRANSPARENCY
    return {"flags": flags, "index": index, "version_num": 1, "enum_id": 0,
            "priority": 6, "transparency": 0, "filename": filename}


def build_environment(riffname, textures=(), scale=None, sound_dir=None):
    env = Chunk("REBENVDT", children=[])
    env.add(Chunk("ENDTHEAD", codecs.encode("ENDTHEAD", {"flags": 0, "version_no": 1})))
    if scale is not None:
        env.add(Chunk("ENVSDSCL", codecs.encode("ENVSDSCL", {"scale": scale})))
    env.add(Chunk("BMPNAMES", codecs.encode("BMPNAMES", {"entries": list(textures)})))
    env.add(Chunk("BMNAMVER", codecs.encode("BMNAMVER", {"value": 1})))
    env.add(Chunk("RIFFNAME", codecs.encode("RIFFNAME", {"name": riffname})))
    if sound_dir:
        env.add(Chunk("SOUNDDIR", codecs.encode("SOUNDDIR", {"name": sound_dir})))
    return env


def _slerp(a, b, t):
    d = sum(x * y for x, y in zip(a, b))
    if d < 0:
        b = [-x for x in b]
        d = -d
    if d > 0.9995:
        r = [x + (y - x) * t for x, y in zip(a, b)]
    else:
        th = math.acos(min(1.0, d))
        s = math.sin(th)
        r = [(math.sin((1 - t) * th) * x + math.sin(t * th) * y) / s for x, y in zip(a, b)]
    n = math.sqrt(sum(x * x for x in r)) or 1.0
    return [x / n for x in r]


def track_is_looping(track):
    """Engine rule (projload.cpp): the last keyframe's segment length is
    65536 - at; if that is <= 6 the sequence does not loop."""
    return (65536 - track.frames[-1].at) > 6


def sample_track(track, at):
    """(quat, offset) of a track at time 'at' (0..65536), interpolated like the
    engine: linear offsets, slerp orientation, last key -> first key when
    looping (hmodel.c Handle_Section_Timer / Slerp)."""
    fr = track.frames
    if len(fr) == 1:
        return list(fr[0].quat), list(fr[0].offset)
    loop = track_is_looping(track)
    for i, f in enumerate(fr):
        last = i == len(fr) - 1
        end = 65536 if last else fr[i + 1].at
        if at < end or last:
            nxt = fr[0] if last and loop else (f if last else fr[i + 1])
            span = end - f.at
            t = 0.0 if nxt is f or span <= 0 else max(0.0, min(1.0, (at - f.at) / float(span)))
            return (_slerp(f.quat, nxt.quat, t),
                    [a + (b - a) * t for a, b in zip(f.offset, nxt.offset)])
    return list(fr[0].quat), list(fr[0].offset)


def reduce_keys(frames, loop, pos_tol=0.5, rot_tol=2e-6):
    """Drop keyframes the engine's interpolation reproduces within tolerance
    (offset in mm, rotation as 1 - |q.q'|). Keeps the first key, keys with
    flags, and the last key of non-looping tracks."""
    if len(frames) <= 2:
        return list(frames)
    keep = [frames[0]]
    i = 0
    n = len(frames)
    while i < n - 1:
        j = i + 2
        best = i + 1
        while j <= n:
            a = frames[i]
            b = frames[j] if j < n else None
            if b is None:
                if not loop:
                    break
                b = Keyframe(65536, frames[0].quat, frames[0].offset)
            ok = True
            for k in range(i + 1, min(j, n)):
                m = frames[k]
                if m.flags:  # flagged keys (delta/events/sounds) are kept
                    ok = False
                    break
                t = (m.at - a.at) / float(b.at - a.at)
                q = _slerp(a.quat, b.quat, t)
                o = [x + (y - x) * t for x, y in zip(a.offset, b.offset)]
                if (1 - abs(sum(x * y for x, y in zip(q, m.quat))) > rot_tol or
                        max(abs(x - y) for x, y in zip(o, m.offset)) > pos_tol):
                    ok = False
                    break
            if not ok:
                break
            best = j
            j += 1
        if best >= n:
            break
        keep.append(frames[best])
        i = best
    if not loop and keep[-1] is not frames[-1]:
        keep.append(frames[-1])
    return keep


# ---------------------------------------------------------------- hierarchy

class Keyframe:
    __slots__ = ("at", "quat", "offset", "flags")

    def __init__(self, at, quat, offset, flags=0):
        self.at = int(at)
        self.quat = list(quat)
        self.offset = [int(round(x)) for x in offset]
        self.flags = flags


class Track:
    """One sequence for one section."""

    def __init__(self, sequence, sub_sequence, frames, time_ms=0, name=""):
        self.sequence = sequence
        self.sub_sequence = sub_sequence
        self.frames = frames
        self.time_ms = time_ms
        self.name = name

    @property
    def key(self):
        return (self.sequence, self.sub_sequence)


class Section:
    def __init__(self, name, children=None, tracks=None, extra=None):
        self.name = name
        self.children = list(children or [])
        self.tracks = list(tracks or [])
        self.extra = list(extra or [])

    def walk(self, depth=0):
        yield depth, self
        for c in self.children:
            yield from c.walk(depth + 1)

    def track(self, key):
        for t in self.tracks:
            if t.key == tuple(key):
                return t
        return None


class Hierarchy:
    def __init__(self, name, root, seq_names=None):
        self.name = name
        self.root = root
        self.seq_names = dict(seq_names or {})  # (seq, sub) -> name

    def sections(self):
        return [s for _, s in self.root.walk()]

    def sequence_keys(self):
        return [t.key for t in self.root.tracks]


def _read_section(node):
    hd = codecs.decode_chunk(node.find("OBJHIERD"))
    sec = Section(hd["object_name"], extra=hd["extra"])
    al = node.find("OBANALLS")
    if al is not None:
        for s in codecs.decode_chunk(al)["sequences"]:
            frames = [Keyframe(f["at_frame_no"], f["quat"], f["offset"], f["flags"])
                      for f in s["frames"]]
            sec.tracks.append(Track(s["sequence"], s["sub_sequence"], frames, s["time_ms"]))
    seqs = node.find("OBANSEQS")
    if seqs is not None:
        for sc in seqs.findall("OBANSEQC"):
            h = codecs.decode_chunk(sc.find("OBASEQHD"))
            frs = [codecs.decode_chunk(f) for f in sc.findall("OBASEQFR")]
            tm = sc.find("OBASEQTM")
            tms = codecs.decode_chunk(tm)["value"] if tm else 0
            key = (h["sequence"], h["sub_sequence"])
            existing = sec.track(key)
            if frs and existing is None:
                frs.sort(key=lambda f: f["frame_ref_no"])
                frames = [Keyframe(f["at_frame_no"], f["quat"], f["offset"], f["flags"])
                          for f in frs]
                sec.tracks.append(Track(key[0], key[1], frames, tms, h["name"]))
            if existing is not None:
                existing.name = h["name"]
    for c in node.findall("OBJCHIER"):
        sec.children.append(_read_section(c))
    return sec


def read_hierarchies(root):
    out = []
    for top in root.findall("OBJCHIER"):
        nm = top.find("OBHIERNM")
        name = codecs.decode_chunk(nm)["name"] if nm else "Template"
        kids = top.findall("OBJCHIER")
        if len(kids) != 1:
            continue
        rsec = _read_section(kids[0])
        names = {t.key: t.name for t in rsec.tracks if t.name}
        # stock files keep the sequence names (OBANSEQS/OBANSEQC/OBASEQHD)
        # on the top-level OBJCHIER, next to OBHIERNM
        seqs = top.find("OBANSEQS")
        if seqs is not None:
            for sc in seqs.findall("OBANSEQC"):
                hd = sc.find("OBASEQHD")
                if hd is not None:
                    h = codecs.decode_chunk(hd)
                    names[(h["sequence"], h["sub_sequence"])] = h["name"]
                    t = rsec.track((h["sequence"], h["sub_sequence"]))
                    if t is not None and not t.name:
                        t.name = h["name"]
        out.append(Hierarchy(name, rsec, names))
    return out


def _section_chunk(sec, is_root, seq_names):
    node = Chunk("OBJCHIER", children=[])
    node.add(Chunk("OBJHIERD", codecs.encode("OBJHIERD", {"extra": sec.extra,
                                                          "object_name": sec.name})))
    seqs = []
    for t in sec.tracks:
        frames = [{"quat": f.quat, "offset": f.offset, "at_frame_no": f.at, "flags": f.flags}
                  for f in t.frames]
        seqs.append({"sequence": t.sequence, "sub_sequence": t.sub_sequence,
                     "time_ms": t.time_ms, "frames": frames})
    node.add(Chunk("OBANALLS", codecs.encode("OBANALLS", {"sequences": seqs})))
    for c in sec.children:
        node.add(_section_chunk(c, False, seq_names))
    return node


def build_hierarchy(h):
    top = Chunk("OBJCHIER", children=[])
    top.add(Chunk("OBHIERNM", codecs.encode("OBHIERNM", {"name": h.name})))
    # sequence names (tools only; the loader reads frames from OBANALLS),
    # stored on the top-level chunk like the stock files do
    ss = Chunk("OBANSEQS", children=[])
    for t in h.root.tracks:
        sc = Chunk("OBANSEQC", children=[])
        sc.add(Chunk("OBASEQHD", codecs.encode("OBASEQHD", {
            "num_frames": ONE_FIXED, "sequence": t.sequence,
            "sub_sequence": t.sub_sequence,
            "name": h.seq_names.get(t.key, t.name or "seq_%d_%d" % t.key)})))
        ss.add(sc)
    top.add(ss)
    top.add(_section_chunk(h.root, True, h.seq_names))
    return top


def pose_sections(h, key, at=0):
    """World (pivot, 3x3 rotation) per section name at a keyframe time
    (nearest earlier keyframe, no interpolation): for previews/tests."""
    out = {}

    def frame_for(sec):
        t = sec.track(key) or (sec.tracks[0] if sec.tracks else None)
        if t is None or not t.frames:
            return [0, 0, 0, 1], [0, 0, 0]
        f = t.frames[0]
        for fr in t.frames:
            if fr.at <= at:
                f = fr
        return f.quat, f.offset

    def rec(sec, pw, pa):
        q, off = frame_for(sec)
        w = [pw[i] + sum(pa[i][k] * off[k] for k in range(3)) for i in range(3)]
        r = quat_to_mat(q)
        a = [[sum(pa[i][k] * r[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
        out[sec.name] = (w, a)
        for c in sec.children:
            rec(c, w, a)

    rec(h.root, [0, 0, 0], [[1, 0, 0], [0, 1, 0], [0, 0, 1]])
    return out


# ---------------------------------------------------------------- files

def objects_by_name(root):
    out = {}
    for o in root.findall("RBOBJECT"):
        h = o.find("OBJHEAD1")
        if h is not None:
            out[codecs.decode_chunk(h)["name"]] = o
    return out


def shapes_by_id(root):
    out = {}
    for s in root.findall("REBSHAPE"):
        h = s.find("SHPHEAD1")
        if h is not None:
            out[codecs.decode_chunk(h)["file_id_num"]] = s
    return out


def next_shape_id(root):
    ids = shapes_by_id(root)
    return max(ids) + 1 if ids else 0


def environment(root):
    return root.find("REBENVDT")


def texture_list(root):
    env = environment(root)
    if env is None or env.find("BMPNAMES") is None:
        return []
    return codecs.decode_chunk(env.find("BMPNAMES"))["entries"]


def set_texture_list(root, entries):
    env = environment(root)
    bn = env.find("BMPNAMES")
    data = codecs.encode("BMPNAMES", {"entries": entries})
    if bn is None:
        env.add(Chunk("BMPNAMES", data))
    else:
        bn.data = data


def new_rif(riffname, textures=(), scale=None, version=1):
    root = Chunk("REBINFF2", children=[])
    root.add(Chunk("RIFVERIN", codecs.encode("RIFVERIN", {"value": version})))
    root.add(build_environment(riffname, textures, scale))
    return root
