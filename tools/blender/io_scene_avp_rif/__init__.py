"""Blender add-on: import/export Aliens versus Predator (Gold) RIF models."""

bl_info = {
    "name": "AvP RIF models",
    "author": "avpmp contributors",
    "version": (0, 1, 0),
    "blender": (4, 2, 0),
    "location": "File > Import/Export > AvP RIF (.rif)",
    "description": "Import and export Aliens versus Predator RIF shapes, "
                   "hierarchies and animations",
    "category": "Import-Export",
}

import os
import sys

import bpy
from bpy.props import BoolProperty, StringProperty
from bpy_extras.io_utils import ExportHelper, ImportHelper

# the pure-Python format library lives next to this package (vendored copy or
# symlink to tools/rif/avprif)
_here = os.path.dirname(os.path.abspath(__file__))
if _here not in sys.path:
    sys.path.insert(0, _here)

from . import exporter, importer  # noqa: E402


def _prefs(context):
    a = context.preferences.addons.get(__package__)
    return a.preferences if a else None


class AVP_Preferences(bpy.types.AddonPreferences):
    bl_idname = __package__

    data_dir: StringProperty(
        name="AvP data directory",
        description="Installed game data (avp_huds/, avp_rifs/, fastfile/, graphics/)",
        subtype="DIR_PATH",
        default=os.path.expanduser("~/avp-data"))

    def draw(self, context):
        self.layout.prop(self, "data_dir")


class AVP_OT_import_rif(bpy.types.Operator, ImportHelper):
    """Import an AvP RIF file"""
    bl_idname = "import_scene.avp_rif"
    bl_label = "Import AvP RIF"
    bl_options = {"REGISTER", "UNDO"}
    filename_ext = ".rif"
    filter_glob: StringProperty(default="*.rif;*.RIF", options={"HIDDEN"})

    hierarchy: StringProperty(name="Hierarchy", description="Only import this hierarchy "
                              "(empty = all hierarchies that have sequences)")
    animations: BoolProperty(name="Animations", default=True)
    object_filter: StringProperty(name="Object filter", description="Non-hierarchy files: "
                                  "only objects whose name contains this text")

    def execute(self, context):
        p = _prefs(context)
        made = importer.load(context, self.filepath, p.data_dir if p else "",
                             self.hierarchy, self.animations, self.object_filter)
        self.report({"INFO"}, "imported %d objects" % len(made))
        return {"FINISHED"}


class AVP_OT_export_rif(bpy.types.Operator, ExportHelper):
    """Export armatures (hierarchies) and meshes to an AvP RIF file"""
    bl_idname = "export_scene.avp_rif"
    bl_label = "Export AvP RIF"
    filename_ext = ".rif"
    filter_glob: StringProperty(default="*.rif", options={"HIDDEN"})

    riffname: StringProperty(name="RIF name", description="REBENVDT/RIFFNAME; must match "
                             "the name the game looks the file up by (e.g. HNPCalien)")
    texture_dir: StringProperty(name="Texture folder", default="custom",
                                description="Folder under graphics/ for exported textures")
    graphics_dir: StringProperty(name="graphics/ directory", subtype="DIR_PATH",
                                 description="Where to write textures (empty = next to "
                                 "the .rif in ../graphics)")
    use_selection: BoolProperty(name="Selected only", default=False)
    compress: BoolProperty(name="Compress (REBCRIF1)", default=False)

    def execute(self, context):
        obs = context.selected_objects if self.use_selection else context.scene.objects
        arms = [o for o in obs if o.type == "ARMATURE"]
        bound = {o for o in context.scene.objects if o.type == "MESH" and any(
            m.type == "ARMATURE" and m.object in arms for m in o.modifiers)}
        statics = [o for o in obs if o.type == "MESH" and o not in bound
                   and not any(m.type == "ARMATURE" for m in o.modifiers)]
        gdir = self.graphics_dir or os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(self.filepath))), "graphics")
        riff = self.riffname or os.path.splitext(os.path.basename(self.filepath))[0]
        try:
            _, _, rep = exporter.export_rif(context, self.filepath, riff, self.texture_dir,
                                           gdir, statics, arms, self.compress)
        except ValueError as e:
            self.report({"ERROR"}, str(e))
            return {"CANCELLED"}
        for line in rep:
            print("AvP export:", line)
        self.report({"INFO"}, "exported %s" % self.filepath)
        return {"FINISHED"}


def _menu_import(self, context):
    self.layout.operator(AVP_OT_import_rif.bl_idname, text="AvP RIF (.rif)")


def _menu_export(self, context):
    self.layout.operator(AVP_OT_export_rif.bl_idname, text="AvP RIF (.rif)")


_classes = (AVP_Preferences, AVP_OT_import_rif, AVP_OT_export_rif)


def register():
    for c in _classes:
        bpy.utils.register_class(c)
    bpy.types.TOPBAR_MT_file_import.append(_menu_import)
    bpy.types.TOPBAR_MT_file_export.append(_menu_export)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(_menu_export)
    bpy.types.TOPBAR_MT_file_import.remove(_menu_import)
    for c in reversed(_classes):
        bpy.utils.unregister_class(c)
