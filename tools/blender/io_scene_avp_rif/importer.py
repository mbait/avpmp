"""Import AvP RIF files into Blender.

* Hierarchy RIFs: an Armature per hierarchy (one bone per section), meshes
  rigidly skinned to their section's bone, one Action per sequence.
* Other RIFs: one mesh object per RBOBJECT (optionally filtered by name).
"""

import bpy
from mathutils import Matrix

from . import avprif  # noqa: F401  (vendored package, see __init__)
from .avprif import chunks, codecs, model, texstore
from .convert import SCALE, matrix_from_avp, quat_from_file, vec_from_avp

SEQ_PROPS = ("avp_sequence", "avp_sub_sequence", "avp_time_ms", "avp_loop")


# ---------------------------------------------------------------- textures

class Materials:
    def __init__(self, root, data_dir, prefix):
        self.store = texstore.TextureStore(data_dir) if data_dir else None
        self.entries = {e["index"]: e for e in model.texture_list(root)}
        self.prefix = prefix
        self.cache = {}

    def get(self, index):
        """-> (material, (w, h))"""
        if index in self.cache:
            return self.cache[index]
        e = self.entries.get(index)
        name = e["filename"] if e else "tex%d" % index
        mat = bpy.data.materials.new("%s%s" % (self.prefix, name.replace("\\", "/")))
        mat["avp_texture"] = name
        mat["avp_texture_flags"] = e["flags"] if e else 0
        size = (128, 128)
        blob = self.store.find(name) if (self.store and e) else None
        mat.use_nodes = True
        if blob:
            try:
                w, h, rgba = texstore.decode_any(blob)
                img = bpy.data.images.new(name.replace("\\", "/"), w, h, alpha=True)
                # Blender pixels are bottom-up float RGBA
                rows = [rgba[(h - 1 - y) * w * 4:(h - y) * w * 4] for y in range(h)]
                flat = b"".join(rows)
                img.pixels.foreach_set([c / 255.0 for c in flat])
                img.pack()
                img["avp_texture"] = name
                size = (w, h)
                nt = mat.node_tree
                bsdf = nt.nodes.get("Principled BSDF")
                tn = nt.nodes.new("ShaderNodeTexImage")
                tn.image = img
                tn.interpolation = "Closest"
                nt.links.new(tn.outputs["Color"], bsdf.inputs["Base Color"])
                if e and e["flags"] & model.BMPN_USES_TRANSPARENCY:
                    nt.links.new(tn.outputs["Alpha"], bsdf.inputs["Alpha"])
            except Exception as ex:  # keep importing without the image
                print("AvP import: texture %s: %s" % (name, ex))
        mat["avp_texture_size"] = list(size)
        self.cache[index] = (mat, size)
        return self.cache[index]


def _mesh_from_shape(name, mesh, mats, world=None):
    """model.Mesh (AvP mm, section/object local) -> bpy mesh (optionally
    transformed by a Blender 4x4 'world')."""
    me = bpy.data.meshes.new(name)
    verts = [vec_from_avp(v) for v in mesh.verts]
    if world is not None:
        verts = [world @ v for v in verts]
    faces = [p.verts for p in mesh.polys]
    me.from_pydata([tuple(v) for v in verts], [], faces)
    me.update()
    uvl = me.uv_layers.new(name="UVMap")
    slot_of = {}
    for pi, p in enumerate(mesh.polys):
        mp = me.polygons[pi]
        if p.texture is not None:
            mat, (w, h) = mats.get(p.texture)
            if mat.name not in slot_of:
                me.materials.append(mat)
                slot_of[mat.name] = len(me.materials) - 1
            mp.material_index = slot_of[mat.name]
            if p.uvs:
                for k, li in enumerate(mp.loop_indices):
                    u, v = p.uvs[k]
                    uvl.data[li].uv = (u / w, 1.0 - v / h)
    # keep per-face engine data so export can round-trip it
    fl = me.attributes.new("avp_flags", "INT", "FACE")
    et = me.attributes.new("avp_engine_type", "INT", "FACE")
    for pi, p in enumerate(mesh.polys):
        fl.data[pi].value = p.flags - (1 << 32) if p.flags >= 1 << 31 else p.flags
        et.data[pi].value = p.engine_type
    return me


# ---------------------------------------------------------------- hierarchy

def _section_world(h, key, frame_index):
    """World (rot3, pos) per section for keyframe 'frame_index' of track key,
    using each section's own frame list."""
    out = {}

    def rec(sec, pt, pr):
        t = sec.track(key) or (sec.tracks[0] if sec.tracks else None)
        if t and t.frames:
            f = t.frames[min(frame_index, len(t.frames) - 1)]
            q, off = f.quat, f.offset
        else:
            q, off = (0, 0, 0, 1), (0, 0, 0)
        from mathutils import Vector
        pos = pt + pr @ Vector(off)
        rot = pr @ quat_from_file(q)
        out[sec.name] = (rot, pos)
        for c in sec.children:
            rec(c, pos, rot)

    from mathutils import Vector
    rec(h.root, Vector((0, 0, 0)), Matrix.Identity(3))
    return out


def _local_at(sec, key, at):
    """Interpolated (rot3 AvP, offset) of a section at time 'at' (0..65536)."""
    t = sec.track(key) or (sec.tracks[0] if sec.tracks else None)
    if t is None or not t.frames:
        return Matrix.Identity(3), (0, 0, 0)
    q, off = model.sample_track(t, at)
    return quat_from_file(q), off


def import_hierarchy(context, root, h, mats, riffname, import_anims=True, lod=False):
    from mathutils import Vector
    objs = model.objects_by_name(root)
    shapes = model.shapes_by_id(root)
    secs = h.sections()
    base_key = h.root.tracks[0].key if h.root.tracks else (0, 0)
    rest = _section_world(h, base_key, 0)

    arm = bpy.data.armatures.new(h.name)
    arm_ob = bpy.data.objects.new(h.name, arm)
    context.collection.objects.link(arm_ob)
    arm_ob["avp_hierarchy"] = h.name
    arm_ob["avp_riffname"] = riffname
    sounds = [codecs.decode_chunk(c) for c in root.findall("INDSOUND")]
    if sounds:
        import json
        arm_ob["avp_sounds"] = json.dumps(sounds)
    env = root.find("REBENVDT")
    if env is not None and env.find("SOUNDDIR") is not None:
        arm_ob["avp_sound_dir"] = codecs.decode_chunk(env.find("SOUNDDIR"))["name"]
    arm.display_type = "STICK"
    context.view_layer.objects.active = arm_ob
    bpy.ops.object.mode_set(mode="EDIT")
    parent_of = {}
    for _, s in h.root.walk():
        for c in s.children:
            parent_of[c.name] = s.name
    for s in secs:
        rot, pos = rest[s.name]
        eb = arm.edit_bones.new(s.name)
        eb.head = (0, 0, 0)
        eb.tail = (0, 0.05, 0)
        eb.matrix = matrix_from_avp(rot, pos)
        if s.name in parent_of:
            eb.parent = arm.edit_bones[parent_of[s.name]]
    bpy.ops.object.mode_set(mode="OBJECT")
    for s in secs:
        arm.bones[s.name]["avp_extra"] = list(s.extra)

    # meshes, rigidly bound to their bone
    for s in secs:
        o = objs.get(s.name)
        if o is None:
            continue
        hd = codecs.decode_chunk(o.find("OBJHEAD1"))
        sh = shapes.get(hd["shape_id_no"])
        if sh is None:
            continue
        mesh, _ = model.read_shape(sh)
        rot, pos = rest[s.name]
        world = matrix_from_avp(rot, pos)
        me = _mesh_from_shape(s.name, mesh, mats, world)
        ob = bpy.data.objects.new(s.name, me)
        context.collection.objects.link(ob)
        ob.parent = arm_ob
        vg = ob.vertex_groups.new(name=s.name)
        vg.add(range(len(me.vertices)), 1.0, "REPLACE")
        mod = ob.modifiers.new("Armature", "ARMATURE")
        mod.object = arm_ob

    if not import_anims:
        return arm_ob

    # actions: one per sequence
    fps = context.scene.render.fps
    arm_ob.animation_data_create()
    arm_ob.rotation_mode = "QUATERNION"
    for pb in arm_ob.pose.bones:
        pb.rotation_mode = "QUATERNION"
    rest_arm = {b.name: b.matrix_local.copy() for b in arm.bones}
    for t in h.root.tracks:
        key = t.key
        name = h.seq_names.get(key) or t.name or "seq_%d_%d" % key
        act = bpy.data.actions.new("%s|%s" % (h.name, name))
        act["avp_sequence"] = key[0]
        act["avp_sub_sequence"] = key[1]
        act["avp_time_ms"] = t.time_ms  # 0 = engine default (caller's choice)
        last = t.frames[-1]
        loop = (65536 - last.at) > 6
        act["avp_loop"] = loop
        act.use_fake_user = True
        dur = max(1.0, (t.time_ms / 1000.0) * fps) if t.time_ms else 30.0
        arm_ob.animation_data.action = act
        # union of keyframe times across sections
        times = {f.at for s in secs for tt in [s.track(key)] if tt for f in tt.frames}
        # plus one key per frame: Blender interpolates quaternion channels
        # linearly while the engine slerps, which diverges on big rotations
        nfr = int(dur)
        times |= {int(round(i * 65536.0 / dur)) for i in range(nfr + 1)}
        times = sorted(t for t in times if t < 65536 or loop)
        if loop and times[-1] != 65536:
            times.append(65536)
        if not loop:
            times = [t for t in times if t <= t_last_max(secs, key)]
        prev_q = {}
        for at in times:
            frame = 1 + at / 65536.0 * dur
            # absolute pose per section in AvP space
            world = {}

            def rec(sec, pt, pr):
                r, off = _local_at(sec, key, at)
                pos = pt + pr @ Vector(off)
                rot = pr @ r
                world[sec.name] = (rot, pos)
                for c in sec.children:
                    rec(c, pos, rot)
            rec(h.root, Vector((0, 0, 0)), Matrix.Identity(3))
            for s in secs:
                pb = arm_ob.pose.bones[s.name]
                cur = matrix_from_avp(*world[s.name])
                if s.name in parent_of:
                    pcur = matrix_from_avp(*world[parent_of[s.name]])
                    prest = rest_arm[parent_of[s.name]]
                    basis = (prest.inverted() @ rest_arm[s.name]).inverted() @ (pcur.inverted() @ cur)
                else:
                    basis = rest_arm[s.name].inverted() @ cur
                loc, rq, _ = basis.decompose()
                # keep sign continuity: Blender interpolates quaternion
                # channels linearly, the engine slerps along the short arc
                pq = prev_q.get(s.name)
                if pq is not None and pq.dot(rq) < 0:
                    rq.negate()
                prev_q[s.name] = rq.copy()
                pb.location = loc
                pb.rotation_quaternion = rq
                pb.keyframe_insert("location", frame=frame, group=s.name)
                pb.keyframe_insert("rotation_quaternion", frame=frame, group=s.name)
        # keyframe flags -> pose markers: "delta" (DeltaFrame on all
        # sections), root event bits / sound index as "flags=0x..."
        delta_at = None
        for s in secs:
            tt = s.track(key)
            for f in (tt.frames if tt else []):
                if f.flags & 0x80000000:
                    delta_at = f.at
        if delta_at is not None:
            mk = act.pose_markers.new("delta")
            mk.frame = int(round(1 + delta_at / 65536.0 * dur))
            act["avp_delta_at"] = delta_at
        rt = h.root.track(key)
        for f in (rt.frames if rt else []):
            fl = f.flags & 0x7FFFFFFF
            if fl:
                mk = act.pose_markers.new("flags=0x%x" % fl)
                mk.frame = int(round(1 + f.at / 65536.0 * dur))
        for fc in _fcurves(act):
            for kp in fc.keyframe_points:
                kp.interpolation = "LINEAR"
        act.frame_range = (1, 1 + dur)
        act.use_frame_range = True
    arm_ob.animation_data.action = None
    return arm_ob


def t_last_max(secs, key):
    m = 0
    for s in secs:
        t = s.track(key)
        if t and t.frames:
            m = max(m, t.frames[-1].at)
    return m


def _fcurves(act):
    if hasattr(act, "fcurves") and len(getattr(act, "fcurves", [])):
        return list(act.fcurves)
    out = []
    for layer in getattr(act, "layers", []):
        for strip in layer.strips:
            for bag in strip.channelbags:
                out.extend(bag.fcurves)
    return out


# ---------------------------------------------------------------- entry

def load(context, filepath, data_dir="", hierarchy="", import_anims=True,
         object_filter="", skip_lod=True):
    root = chunks.load(filepath)
    env = root.find("REBENVDT")
    riffname = ""
    if env is not None and env.find("RIFFNAME") is not None:
        riffname = codecs.decode_chunk(env.find("RIFFNAME"))["name"]
    mats = Materials(root, data_dir, "")
    hs = model.read_hierarchies(root)
    hs = [h for h in hs if h.root.tracks]
    if hierarchy:
        hs = [h for h in hs if h.name == hierarchy]
    made = []
    if hs:
        for h in hs:
            made.append(import_hierarchy(context, root, h, mats, riffname, import_anims))
        return made
    shapes = model.shapes_by_id(root)
    for name, o in model.objects_by_name(root).items():
        if skip_lod and len(name) > 2 and name[0] == "L" and name[2] == "#":
            continue
        if object_filter and object_filter.lower() not in name.lower():
            continue
        hd = codecs.decode_chunk(o.find("OBJHEAD1"))
        sh = shapes.get(hd["shape_id_no"])
        if sh is None:
            continue
        mesh, _ = model.read_shape(sh)
        me = _mesh_from_shape(name, mesh, mats)
        ob = bpy.data.objects.new(name, me)
        # placed objects use the conjugate quaternion (chnkload.cpp)
        q = hd["orientation"]
        r = quat_from_file((-q[0], -q[1], -q[2], q[3]))
        ob.matrix_world = matrix_from_avp(r, hd["location"])
        ob["avp_object"] = name
        ob["avp_riffname"] = riffname
        context.collection.objects.link(ob)
        made.append(ob)
    return made
