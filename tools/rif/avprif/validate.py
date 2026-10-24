"""Static checks that mirror the engine's hard requirements (asserts, exits,
unchecked dereferences) when it loads a RIF. See docs/RESOURCES.md,
"Loader requirements".

check(root, kind) -> list of (level, message), level in {"error", "warning"}.
kind: "level" (avp_rifs/*.rif), "hierarchy" (NPC / HUD / placed hierarchy rifs),
      "hud" (hierarchy + "Camera Root" section), or "shapes" (shape-only rifs).
"""

from . import codecs
from .model import read_hierarchies

TEXTURED = {5, 6, 7, 21, 22, 23, 24, 25}


def _dec(ch):
    return codecs.decode_chunk(ch)


def check(root, kind="hierarchy"):
    out = []

    def err(m):
        out.append(("error", m))

    def warn(m):
        out.append(("warning", m))

    env = root.find("REBENVDT")
    if env is None:
        err("missing REBENVDT (the loader dereferences it unconditionally)")
    elif env.find("RIFFNAME") is None and kind != "level":
        err("REBENVDT has no RIFFNAME (required by Global_Hierarchy_Store)")
    texidx = set()
    if env is not None and env.find("BMPNAMES") is not None:
        for e in _dec(env.find("BMPNAMES"))["entries"]:
            texidx.add(e["index"])
            if not e["flags"] & 0x1000000 and not e["flags"] & 0x800:
                warn("texture %r lacks the IFF (relative path) flag; the PC build "
                     "resolves it to a network path and fails" % e["filename"])
            if e["index"] > 0xFFF:
                err("texture index %d > 4095" % e["index"])

    # shapes
    ids = {}
    for i, sh in enumerate(root.findall("REBSHAPE")):
        h = sh.find("SHPHEAD1")
        if h is None:
            err("REBSHAPE #%d has no SHPHEAD1" % i)
            continue
        hd = _dec(h)
        fid = hd["file_id_num"]
        if fid < 0:
            err("REBSHAPE #%d has file_id_num %d" % (i, fid))
        if fid in ids:
            err("duplicate shape file_id_num %d" % fid)
        ids[fid] = sh
        for need in ("SHPRAWVT", "SHPPOLYS", "SHPVNORM", "SHPPNORM"):
            if sh.find(need) is None and sh.find("SHPPRPRO") is None:
                err("shape %d lacks %s" % (fid, need))
        if sh.find("SHPRAWVT") is None or sh.find("SHPPOLYS") is None:
            continue
        nv = len(_dec(sh.find("SHPRAWVT"))["verts"])
        polys = _dec(sh.find("SHPPOLYS"))["polys"]
        uvl = _dec(sh.find("SHPUVCRD"))["uv_lists"] if sh.find("SHPUVCRD") else []
        if nv != hd["num_verts"] or len(polys) != hd["num_polys"]:
            err("shape %d header counts (%d v, %d p) != data (%d v, %d p)"
                % (fid, hd["num_verts"], hd["num_polys"], nv, len(polys)))
        npn = len(_dec(sh.find("SHPPNORM"))["normals"]) if sh.find("SHPPNORM") else 0
        nvn = len(_dec(sh.find("SHPVNORM"))["normals"]) if sh.find("SHPVNORM") else 0
        if nvn and nvn != nv:
            err("shape %d has %d vertex normals for %d vertices" % (fid, nvn, nv))
        for pi, p in enumerate(polys):
            if not 3 <= len(p["verts"]) <= 4:
                err("shape %d poly %d has %d vertices (3 or 4 allowed)" % (fid, pi, len(p["verts"])))
            if any(not 0 <= v < nv for v in p["verts"]):
                err("shape %d poly %d references a missing vertex" % (fid, pi))
            if p["normal_index"] >= npn:
                err("shape %d poly %d normal index out of range" % (fid, pi))
            if p["engine_type"] in TEXTURED:
                t = p["colour"] & 0xFFF
                ui = p["colour"] >> 16
                if p["colour"] & 0xF000:
                    warn("shape %d poly %d uses colour bits 12-15 (ignored by the game)" % (fid, pi))
                if ui >= len(uvl) or len(uvl[ui]) != len(p["verts"]):
                    warn("shape %d poly %d has no matching UV list (drawn as a line)" % (fid, pi))
                if texidx and t not in texidx and sh.find("SHPEXTFL") is None:
                    warn("shape %d poly %d uses texture %d not in BMPNAMES" % (fid, pi, t))
        if hd["num_verts"] == 0:
            warn("shape %d is empty" % fid)

    # objects
    names = {}
    for o in root.findall("RBOBJECT"):
        h = o.find("OBJHEAD1")
        if h is None:
            err("RBOBJECT without OBJHEAD1")
            continue
        d = _dec(h)
        if d["shape_id_no"] not in ids:
            err("object %r references missing shape %d" % (d["name"], d["shape_id_no"]))
        if len(d["name"]) > 49:
            err("object name %r longer than 49 characters" % d["name"])
        names[d["name"]] = d

    # hierarchies
    if kind in ("hierarchy", "hud"):
        tops = root.findall("OBJCHIER")
        if not tops:
            err("no OBJCHIER hierarchy chunk")
        for top in tops:
            kids = top.findall("OBJCHIER")
            if len(kids) != 1:
                err("top-level OBJCHIER must have exactly one child section (has %d)" % len(kids))
            if top.find("OBHIERNM") is None:
                warn("hierarchy without OBHIERNM is named 'Template'")
        for h in read_hierarchies(root):
            secs = h.sections()
            keys = h.sequence_keys()
            if not keys:
                warn("hierarchy %r root has no sequences: the loader skips it" % h.name)
                continue
            for s in secs:
                if not s.name:
                    err("hierarchy %r has a section with an empty name" % h.name)
                if s.name not in names and not s.name.lower().startswith(("dum", "camera", "root")):
                    warn("hierarchy %r section %r has no RBOBJECT of that name "
                         "(it will have no geometry)" % (h.name, s.name))
                if not s.tracks:
                    err("hierarchy %r section %r has no sequences (Preprocess_HModel "
                        "reads sequence_array[0])" % (h.name, s.name))
                skeys = [t.key for t in s.tracks]
                if skeys != keys:
                    warn("hierarchy %r section %r sequence set/order differs from root"
                         % (h.name, s.name))
                for t in s.tracks:
                    fr = t.frames
                    tag = "%r/%r seq %s" % (h.name, s.name, t.key)
                    if not fr:
                        err(tag + " has no keyframes")
                        continue
                    if fr[0].at != 0:
                        err(tag + " first keyframe must be at 0")
                    for a, b in zip(fr, fr[1:]):
                        if b.at <= a.at:
                            err(tag + " keyframe times must strictly increase")
                            break
                    if fr[-1].at > 65535:
                        err(tag + " keyframe time > 65535")
                    if sum(1 for f in fr if f.flags & 0x80000000) > 1:
                        err(tag + " has more than one delta frame")
                    for f in fr:
                        if any(abs(c) > 32767 for c in f.offset):
                            warn(tag + " offset beyond +-32767 loses 4 bits of precision")
                            break
                        n = sum(c * c for c in f.quat)
                        if abs(n - 1) > 1e-3:
                            warn(tag + " quaternion not normalised")
                            break
            if kind == "hud" and not any(s.name == "Camera Root" for s in secs):
                err("HUD hierarchy %r lacks a 'Camera Root' section (weapons.c asserts)" % h.name)
    return out
