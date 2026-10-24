"""Headless RIF export.

    blender -b scene.blend --python export_rif.py -- OUT.rif [options]

Options:
    --riffname NAME      REBENVDT/RIFFNAME (default: output file stem)
    --texture-dir DIR    folder under graphics/ for textures (default "custom")
    --graphics DIR       where to write textures (default <OUT dir>/../graphics)
    --armature NAME      export only this armature (repeatable)
    --static NAME        export this mesh object as a static shape (repeatable;
                         default: every mesh not bound to an armature)
    --no-static          do not export static meshes
    --compress           write REBCRIF1 (Huffman) instead of plain REBINFF2
    --check KIND         run rifcheck validation (level|hierarchy|hud|shapes)
"""

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bpy  # noqa: E402

from io_scene_avp_rif import exporter  # noqa: E402
from io_scene_avp_rif.avprif import validate  # noqa: E402


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser(prog="export_rif.py")
    ap.add_argument("out")
    ap.add_argument("--riffname")
    ap.add_argument("--texture-dir", default="custom")
    ap.add_argument("--graphics")
    ap.add_argument("--armature", action="append", default=[])
    ap.add_argument("--static", action="append", default=[])
    ap.add_argument("--no-static", action="store_true")
    ap.add_argument("--compress", action="store_true")
    ap.add_argument("--check")
    a = ap.parse_args(argv)

    scene = bpy.context.scene
    arms = [o for o in scene.objects if o.type == "ARMATURE"]
    if a.armature:
        arms = [o for o in arms if o.name in a.armature]
    bound = {o for o in scene.objects if o.type == "MESH" and any(
        m.type == "ARMATURE" for m in o.modifiers)}
    if a.no_static:
        statics = []
    elif a.static:
        statics = [scene.objects[n] for n in a.static]
    else:
        statics = [o for o in scene.objects if o.type == "MESH" and o not in bound]
    out = os.path.abspath(a.out)
    gdir = a.graphics or os.path.join(os.path.dirname(os.path.dirname(out)), "graphics")
    riff = a.riffname or os.path.splitext(os.path.basename(out))[0]
    root, bank, rep = exporter.export_rif(bpy.context, out, riff, a.texture_dir, gdir,
                                          statics, arms, a.compress)
    for line in rep:
        print("export:", line)
    if a.check:
        res = validate.check(root, a.check)
        for lvl, m in res:
            print("check %s: %s" % (lvl, m))
        if any(lvl == "error" for lvl, _ in res):
            sys.exit(1)
        print("check: ok (%d warnings)" % len(res))


if __name__ == "__main__":
    main()
