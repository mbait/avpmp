"""Quick preview renders (used by the headless scripts and tests)."""

import math

import bpy
from mathutils import Vector


def _bounds(objs):
    pts = []
    dg = bpy.context.evaluated_depsgraph_get()
    for o in objs:
        if o.type != "MESH":
            continue
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        mw = o.matrix_world
        pts += [mw @ v.co for v in me.vertices]
        ev.to_mesh_clear()
    if not pts:
        return Vector((0, 0, 0)), 1.0
    mn = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    mx = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return (mn + mx) / 2, max((mx - mn).length, 0.1)


def render_preview(context, path, res=(800, 800), view=(-0.55, -1.0, 0.45),
                   engine="BLENDER_EEVEE_NEXT"):
    scene = context.scene
    centre, size = _bounds(list(scene.objects))
    cam_data = bpy.data.cameras.new("avp_preview_cam")
    cam = bpy.data.objects.new("avp_preview_cam", cam_data)
    scene.collection.objects.link(cam)
    d = Vector(view).normalized()
    cam.location = centre + d * size * 1.6
    cam.rotation_euler = (centre - cam.location).to_track_quat("-Z", "Y").to_euler()
    cam_data.lens = 50
    scene.camera = cam
    sun_d = bpy.data.lights.new("avp_preview_sun", "SUN")
    sun_d.energy = 3.0
    sun = bpy.data.objects.new("avp_preview_sun", sun_d)
    sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
    scene.collection.objects.link(sun)
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.color = (0.25, 0.25, 0.28)
    try:
        scene.render.engine = engine
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.film_transparent = False
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("rendered", path)
