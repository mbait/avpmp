"""Headless RIF import.

    blender -b [scene.blend] --python import_rif.py -- FILE.rif [options]

Options:
    --data-dir DIR      game data (for textures), default ~/avp-data
    --hierarchy NAME    only this hierarchy
    --no-anims          skip animations
    --filter TEXT       non-hierarchy files: object name filter
    --save OUT.blend    save the resulting scene
    --render OUT.png    render a preview (camera framing all imported objects)
    --clear             delete existing objects first (e.g. the default cube)
    --frame N           frame to render
    --action NAME       action to assign to imported armatures before rendering
"""

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bpy  # noqa: E402

import io_scene_avp_rif  # noqa: E402
from io_scene_avp_rif import importer, preview  # noqa: E402


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser(prog="import_rif.py")
    ap.add_argument("rif")
    ap.add_argument("--data-dir", default=os.path.expanduser("~/avp-data"))
    ap.add_argument("--hierarchy", default="")
    ap.add_argument("--no-anims", action="store_true")
    ap.add_argument("--filter", default="")
    ap.add_argument("--save")
    ap.add_argument("--render")
    ap.add_argument("--frame", type=int, default=1)
    ap.add_argument("--action", default="")
    ap.add_argument("--clear", action="store_true", help="delete existing objects first")
    a = ap.parse_args(argv)
    if a.clear:
        for o in list(bpy.data.objects):
            bpy.data.objects.remove(o)
    try:
        io_scene_avp_rif.register()
    except ValueError:
        pass  # already registered
    made = importer.load(bpy.context, a.rif, a.data_dir, a.hierarchy, not a.no_anims, a.filter)
    print("imported", len(made), "objects:", [o.name for o in made][:10])
    if a.action:
        act = bpy.data.actions.get(a.action) or next(
            (x for x in bpy.data.actions if x.name.endswith(a.action)), None)
        for o in made:
            if o.type == "ARMATURE" and act:
                o.animation_data.action = act
                if hasattr(act, "slots") and len(act.slots):
                    o.animation_data.action_slot = act.slots[0]
    bpy.context.scene.frame_set(a.frame)
    if a.save:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(a.save))
    if a.render:
        preview.render_preview(bpy.context, os.path.abspath(a.render))


if __name__ == "__main__":
    main()
