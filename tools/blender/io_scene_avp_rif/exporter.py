"""Export Blender scenes to AvP RIF chunk data.

Conventions (see docs/RESOURCES.md, "Authoring in Blender"):
  * Armature object = one hierarchy. Name from custom property
    "avp_hierarchy" (else the object name). Every bone is a section; the
    bone name is the section name (and the RBOBJECT name for its geometry).
  * Meshes parented to the armature with an Armature modifier and vertex
    groups named after bones are split per bone (rigid skinning: each face
    goes to the bone with the largest total weight of its vertices).
  * Each Action with an "avp_sequence" property is a sequence:
        avp_sequence / avp_sub_sequence  (ints, see src/avp/sequnces.h)
        avp_loop                         (bool, default True)
        avp_time_ms                      (optional; default from frame range)
        avp_aliases                      ("t:s, t:s": export the same
                                          animation under more ids)
    Order: property "avp_order" then (sequence, sub). The first one is the
    section default (sequence_array[0]).
    Action pose markers attach keyframe flags: "delta" (reference frame of a
    delta sequence, flags all sections), "flags=0xN", "event=0xN" (bits 0-23),
    "sound=N" (INDSOUND index, bits 24-30) on the root section.
  * Plain mesh objects (no armature) export as an RBOBJECT + REBSHAPE pair
    with geometry in the object's local space (object scale applied).
  * Materials: the first Image Texture node's image becomes a texture under
    graphics/<texture_dir>/ (written as 24-bit BMP); materials without an
    image get a generated solid-colour 8x8 texture.
"""

import math
import os

import bpy
from mathutils import Matrix

from .avprif import chunks, codecs, image, model
from .convert import SCALE, matrix_to_avp, quat_to_file, vec_to_avp

MAX_AT = 65535


class TextureBank:
    def __init__(self, texture_dir, first_index=1):
        self.texture_dir = texture_dir.strip("\\/")
        self.next = first_index
        self.by_key = {}
        self.entries = []
        self.files = {}  # relative path (with backslashes) -> bmp bytes

    def for_material(self, mat):
        """-> (bmpnames index, (w, h))"""
        key = mat.name if mat else "<none>"
        if key in self.by_key:
            return self.by_key[key]
        img = None
        if mat and mat.use_nodes:
            for n in mat.node_tree.nodes:
                if n.type == "TEX_IMAGE" and n.image is not None:
                    img = n.image
                    break
        safe = "".join(c if c.isalnum() or c in "_-" else "_" for c in key).lower()
        if img is not None and img.size[0] > 0:
            w, h = img.size
            px = list(img.pixels)
            rgba = bytearray(w * h * 4)
            for y in range(h):
                src = (h - 1 - y) * w * 4
                for x in range(w * 4):
                    rgba[y * w * 4 + x] = max(0, min(255, int(round(px[src + x] * 255))))
            rgba = bytes(rgba)
        else:
            w = h = 8
            col = (0.8, 0.8, 0.8, 1.0)
            if mat is not None:
                col = tuple(mat.diffuse_color)
            c = bytes(max(0, min(255, int(v * 255))) for v in col[:3]) + b"\xff"
            rgba = c * (w * h)
        rel = "%s\\%s.bmp" % (self.texture_dir, safe) if self.texture_dir else safe + ".bmp"
        transparent = bool(mat and mat.get("avp_transparent", False))
        idx = self.next
        self.next += 1
        self.entries.append(model.texture_entry(idx, rel, transparent))
        self.files[rel] = image.write_bmp24(w, h, rgba)
        self.by_key[key] = (idx, (w, h))
        return self.by_key[key]

    def write(self, graphics_dir):
        out = []
        for rel, data in self.files.items():
            p = os.path.join(graphics_dir, *rel.lower().split("\\"))
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "wb") as f:
                f.write(data)
            out.append(p)
        return out


def _face_data(me, pi, attr_flags, attr_type):
    fl = attr_flags.data[pi].value & 0xFFFFFFFF if attr_flags else None
    et = attr_type.data[pi].value if attr_type else None
    return fl, et


def _mesh_polys(me, xform, bank, faces=None, materials=None):
    """Triangulated/quad faces of an evaluated mesh -> model.Mesh.

    xform maps mesh coordinates to the section/object frame (Blender units)."""
    me.calc_loop_triangles()
    uvl = me.uv_layers.active
    attr_flags = me.attributes.get("avp_flags")
    attr_type = me.attributes.get("avp_engine_type")
    mesh = model.Mesh()
    vmap = {}

    def vid(i):
        if i not in vmap:
            vmap[i] = len(mesh.verts)
            mesh.verts.append(vec_to_avp(xform @ me.vertices[i].co))
        return vmap[i]

    for poly in me.polygons:
        if faces is not None and poly.index not in faces:
            continue
        mat = None
        if materials and poly.material_index < len(materials):
            mat = materials[poly.material_index]
        tex, (w, h) = bank.for_material(mat)
        fl, et = _face_data(me, poly.index, attr_flags, attr_type)
        if poly.loop_total in (3, 4):
            loops_sets = [list(poly.loop_indices)]
        else:  # fan-free triangulation from Blender's loop triangles
            loops_sets = [list(t.loops) for t in me.loop_triangles if t.polygon_index == poly.index]
        for loops in loops_sets:
            verts = [vid(me.loops[li].vertex_index) for li in loops]
            uvs = []
            for li in loops:
                if uvl is not None:
                    u, v = uvl.data[li].uv
                else:
                    u, v = 0.0, 0.0
                uvs.append((max(0.0, min(w - 0.001, u * w)), max(0.0, min(h - 0.001, (1.0 - v) * h))))
            p = model.Polygon(verts, uvs, tex,
                              et if et in (7, 24) else model.I_3D_TEXTURED_POLYGON,
                              fl if fl is not None else model.IFLAG_TX2DOR3D | model.IFLAG_LINEAR_S)
            if mat is not None and mat.get("avp_double_sided", False):
                p.flags |= model.IFLAG_NO_BFC
            mesh.polys.append(p)
    return mesh


# ---------------------------------------------------------------- statics

def export_static(objects, bank, depsgraph, first_shape_id=0):
    """-> list of (RBOBJECT, REBSHAPE) chunk pairs for plain mesh objects."""
    out = []
    sid = first_shape_id
    for ob in objects:
        ev = ob.evaluated_get(depsgraph)
        me = ev.to_mesh()
        scale = Matrix.Diagonal(ob.matrix_world.to_scale()).to_4x4()
        mesh = _mesh_polys(me, scale, bank, materials=[s.material for s in ob.material_slots])
        ev.to_mesh_clear()
        name = ob.get("avp_object", ob.name)
        out.append((model.build_object(name, sid), model.build_shape(mesh, sid, [name])))
        sid += 1
    return out


# ---------------------------------------------------------------- hierarchy

def _sequence_actions(arm_ob):
    acts = [a for a in bpy.data.actions if "avp_sequence" in a.keys()]
    hname = arm_ob.get("avp_hierarchy", arm_ob.name)
    mine = [a for a in acts if a.get("avp_hierarchy", hname) == hname]
    mine.sort(key=lambda a: (a.get("avp_order", 1 << 30), a["avp_sequence"],
                             a.get("avp_sub_sequence", 0), a.name))
    return mine


def _fcurves(act):
    if hasattr(act, "fcurves") and len(getattr(act, "fcurves", [])):
        return list(act.fcurves)
    out = []
    for layer in getattr(act, "layers", []):
        for strip in layer.strips:
            for bag in strip.channelbags:
                out.extend(bag.fcurves)
    return out


def _assign_action(arm_ob, act):
    ad = arm_ob.animation_data or arm_ob.animation_data_create()
    ad.action = act
    if hasattr(ad, "action_slot") and ad.action_slot is None and hasattr(act, "slots"):
        if len(act.slots):
            ad.action_slot = act.slots[0]


def export_hierarchy(context, arm_ob, bank, first_shape_id=0, sample_step=1.0,
                     reduce=True):
    """-> (hierarchy chunk, [(RBOBJECT, REBSHAPE)], warnings)"""
    warnings = []
    arm = arm_ob.data
    hname = arm_ob.get("avp_hierarchy", arm_ob.name)
    bones = list(arm.bones)
    roots = [b for b in bones if b.parent is None]
    if len(roots) != 1:
        raise ValueError("armature %s must have exactly one root bone (has %d)"
                         % (arm_ob.name, len(roots)))
    rest = {b.name: b.matrix_local.copy() for b in bones}

    # geometry: faces grouped by dominant bone
    depsgraph = context.evaluated_depsgraph_get()
    per_bone = {}
    meshes = [o for o in context.scene.objects if o.type == "MESH" and
              any(m.type == "ARMATURE" and m.object == arm_ob for m in o.modifiers)]
    saved_pose = arm.pose_position
    arm.pose_position = "REST"
    context.view_layer.update()
    depsgraph = context.evaluated_depsgraph_get()
    for ob in meshes:
        groups = {g.index: g.name for g in ob.vertex_groups}
        # rest-pose coordinates with other modifiers (mirror, ...) applied
        ev = ob.evaluated_get(depsgraph)
        me = ev.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        mats = [s.material for s in ob.material_slots]
        faces_of = {}
        for poly in me.polygons:
            score = {}
            for vi in poly.vertices:
                for g in me.vertices[vi].groups:
                    n = groups.get(g.group)
                    if n in rest:
                        score[n] = score.get(n, 0.0) + g.weight
            if not score:
                warnings.append("%s: face %d has no bone weights; skipped" % (ob.name, poly.index))
                continue
            b = max(score, key=score.get)
            faces_of.setdefault(b, set()).add(poly.index)
        to_arm = arm_ob.matrix_world.inverted() @ ob.matrix_world
        for bname, faces in faces_of.items():
            xform = rest[bname].inverted() @ to_arm
            m = _mesh_polys(me, xform, bank, faces, mats)
            acc = per_bone.setdefault(bname, model.Mesh())
            off = len(acc.verts)
            acc.verts += m.verts
            for p in m.polys:
                p.verts = [v + off for v in p.verts]
                acc.polys.append(p)
        ev.to_mesh_clear()
    arm.pose_position = saved_pose

    pairs = []
    sid = first_shape_id
    for b in bones:
        if b.name in per_bone and per_bone[b.name].polys:
            pairs.append((model.build_object(b.name, sid),
                          model.build_shape(per_bone[b.name], sid, [b.name])))
            sid += 1

    # animation
    acts = _sequence_actions(arm_ob)
    if not acts:
        raise ValueError("no actions with an 'avp_sequence' property for %s" % hname)
    scene = context.scene
    fps = scene.render.fps / scene.render.fps_base
    prev_action = arm_ob.animation_data.action if arm_ob.animation_data else None
    prev_frame = scene.frame_current
    tracks = {b.name: [] for b in bones}
    seq_names = {}
    for act in acts:
        key = (int(act["avp_sequence"]), int(act.get("avp_sub_sequence", 0)))
        # "avp_aliases": "type:sub, type:sub" -> same animation under more ids
        aliases = []
        for tok in str(act.get("avp_aliases", "")).replace(";", ",").split(","):
            tok = tok.strip()
            if tok:
                t_, s_ = tok.split(":")
                aliases.append((int(t_), int(s_)))
        loop = bool(act.get("avp_loop", True))
        f0, f1 = act.frame_range
        if f1 <= f0:
            f1 = f0 + 1
        span = f1 - f0
        if "avp_time_ms" in act.keys() and int(act["avp_time_ms"]) >= 0:
            time_ms = int(act["avp_time_ms"])  # explicit (0 = code-driven speed)
        else:
            time_ms = int(round(span / fps * 1000))
        _assign_action(arm_ob, act)
        n = max(1, int(math.ceil(span / sample_step)))
        grid = {round(f0 + i * span / n, 4) for i in range(n)}
        # also sample exactly at authored keyframes so their timing survives
        keyed = set()
        for fc in _fcurves(act):
            for kp in fc.keyframe_points:
                fr = round(kp.co[0], 4)
                if f0 <= fr < f1 or (not loop and fr == f1):
                    keyed.add(fr)
        # pose markers: "delta", "flags=0xN", "sound=N", "event=0xN"
        delta_f = None
        flag_at = {}
        for mk in getattr(act, "pose_markers", []):
            nm = mk.name.strip().lower()
            fr = float(mk.frame)
            if nm == "delta":
                delta_f = fr
            elif "=" in nm:
                k, v = nm.split("=", 1)
                try:
                    v = int(v, 0)
                except ValueError:
                    warnings.append("%s: bad marker %r" % (act.name, mk.name))
                    continue
                bits = {"flags": v, "event": v & 0xFFFFFF, "sound": (v & 0x7F) << 24}.get(k)
                if bits is None:
                    warnings.append("%s: unknown marker %r" % (act.name, mk.name))
                    continue
                flag_at[fr] = flag_at.get(fr, 0) | bits
            else:
                continue
        # markers sit on whole frames; snap each onto exactly one sample
        marks = set(flag_at)
        if delta_f is not None:
            marks.add(delta_f)
        if loop:  # the end of a looping action is its start
            for d in (flag_at, ):
                for mf in list(d):
                    if mf >= f1:
                        d[f0] = d.get(f0, 0) | d.pop(mf)
            if delta_f is not None and delta_f >= f1:
                delta_f = f0
            marks = {f0 if m >= f1 else m for m in marks}
        marks = {min(max(m, f0), f1) for m in marks}
        grid = {x for x in grid if all(abs(x - m) >= 0.5 for m in marks)}
        samples = sorted(grid | keyed | marks | ({f1} if not loop else set()))
        frames = {b.name: [] for b in bones}
        prevq = {}
        for f in samples:
            fi = int(math.floor(f))
            scene.frame_set(fi, subframe=f - fi)
            if loop:
                at = int(round((f - f0) / span * 65536))
            else:
                at = int(round((f - f0) / span * MAX_AT))
            at = min(at, MAX_AT)
            pose = {pb.name: pb.matrix.copy() for pb in arm_ob.pose.bones}
            for b in bones:
                cur = pose[b.name]
                local = (pose[b.parent.name].inverted() @ cur) if b.parent else cur
                r, t = matrix_to_avp(local)
                q = quat_to_file(r)
                pq = prevq.get(b.name)
                if pq is not None and sum(x * y for x, y in zip(pq, q)) < 0:
                    q = [-x for x in q]
                prevq[b.name] = q
                fl = 0
                if delta_f is not None and f == min(max(delta_f, f0), f1):
                    fl |= 0x80000000
                if b.parent is None:
                    for mf, bits in flag_at.items():
                        if f == min(max(mf, f0), f1):
                            fl |= bits
                frames[b.name].append(model.Keyframe(at, q, t, fl))
        # de-duplicate equal times (very short actions)
        for b in bones:
            fr = []
            for k in frames[b.name]:
                if fr and k.at <= fr[-1].at:
                    fr[-1].flags |= k.flags  # merge, never lose markers
                    continue
                fr.append(k)
            if loop and fr and 65536 - fr[-1].at <= 6:
                warnings.append("%s: looping action too short to loop" % act.name)
            if reduce:
                fr = model.reduce_keys(fr, loop)
            tracks[b.name].append(model.Track(key[0], key[1], fr, time_ms, act.name))
            for ak in aliases:
                tracks[b.name].append(model.Track(ak[0], ak[1], fr, time_ms, act.name))
        seq_names[key] = act.name.split("|")[-1]
        for ak in aliases:
            seq_names[ak] = "%s (alias)" % seq_names[key]
    if arm_ob.animation_data:
        arm_ob.animation_data.action = prev_action
    scene.frame_set(prev_frame)

    def build(b):
        s = model.Section(b.name, extra=list(b.get("avp_extra", [])))
        s.tracks = tracks[b.name]
        s.children = [build(c) for c in b.children]
        return s

    h = model.Hierarchy(hname, build(roots[0]), seq_names)
    return model.build_hierarchy(h), pairs, warnings


# ---------------------------------------------------------------- entry

def export_rif(context, filepath, riffname, texture_dir, graphics_dir=None,
               objects=None, armatures=None, compress=False):
    """Write a complete RIF (+ textures). Returns (root chunk, report lines)."""
    bank = TextureBank(texture_dir)
    report = []
    root = model.new_rif(riffname)
    depsgraph = context.evaluated_depsgraph_get()
    sid = 0
    hier_chunks = []
    for arm_ob in armatures or []:
        hc, pairs, warns = export_hierarchy(context, arm_ob, bank, sid)
        report += warns
        for ob_c, sh_c in pairs:
            root.add(sh_c)
            root.add(ob_c)
            sid += 1
        hier_chunks.append(hc)
    for ob_c, sh_c in export_static(objects or [], bank, depsgraph, sid):
        root.add(sh_c)
        root.add(ob_c)
        sid += 1
    for hc in hier_chunks:
        root.add(hc)
    # keyframe sound tables (INDSOUND at file level, SOUNDDIR in REBENVDT)
    import json
    seen = set()
    for arm_ob in armatures or []:
        for snd in json.loads(arm_ob.get("avp_sounds", "[]")):
            if snd["index"] in seen:
                continue
            seen.add(snd["index"])
            root.add(chunks.Chunk("INDSOUND", codecs.encode("INDSOUND", snd)))
        sd = arm_ob.get("avp_sound_dir")
        env = root.find("REBENVDT")
        if sd and env.find("SOUNDDIR") is None:
            env.add(chunks.Chunk("SOUNDDIR", codecs.encode("SOUNDDIR", {"name": sd})))
    model.set_texture_list(root, bank.entries)
    if filepath:
        chunks.save(root, filepath, compress=compress)
        report.append("wrote %s" % filepath)
    if graphics_dir:
        for p in bank.write(graphics_dir):
            report.append("wrote %s" % p)
    return root, bank, report
