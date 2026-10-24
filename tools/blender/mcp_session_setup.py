"""Blender startup script used by start_blender_mcp.sh.

Enables the blender-mcp bridge (auto-starting its socket server) and the
io_scene_avp_rif add-on from this repository.
"""
import os
import sys

import addon_utils
import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

for mod in ("blender_mcp", "io_scene_avp_rif"):
    try:
        addon_utils.enable(mod, default_set=True, persistent=True)
        print("enabled add-on", mod)
    except Exception as e:  # keep going: MCP is still useful without ours
        print("could not enable", mod, e)


def _start():
    scn = bpy.context.scene
    if scn is None:
        return 0.5
    if hasattr(scn, "blendermcp_auto_start_server"):
        scn.blendermcp_auto_start_server = True
    srv = getattr(bpy.types, "blendermcp_server", None)
    if srv is None or not srv.running:
        try:
            bpy.ops.blendermcp.start_server()
        except Exception as e:
            print("blender-mcp start failed:", e)
            return 1.0
    print("blender-mcp server running")
    return None


bpy.app.timers.register(_start, first_interval=1.0)
