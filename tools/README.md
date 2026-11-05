# AvP resource tools

Tools for reading, writing and authoring Aliens versus Predator (Gold)
resources, aimed at building an open replacement resource pack. This
iteration covers **3D models**; the formats are documented in
[`docs/RESOURCES.md`](../docs/RESOURCES.md).

Everything here is plain Python 3 (no third-party packages) plus a Blender
4.2+ add-on. License: MIT (see `LICENSE`); the sample models in `samples/`
are original work under the same license.

```
tools/
  disc/avp_install.py        install the retail CD data for the Linux port
  rif/avprif/                format library (chunks, codecs, Huffman, fastfiles, images, model, validation)
  rif/rifdump.py             print a RIF's chunk tree (decoded with -d / -i)
  rif/rifcheck.py            check RIFs against the engine's loader requirements
  rif/rifpatch.py            replace hierarchies or object shapes inside existing RIFs
  rif/rifcompare.py          compare two hierarchy RIFs (geometry, poses over time)
  rif/tests/                 unit + corpus tests
  blender/io_scene_avp_rif/  Blender add-on: File > Import/Export > AvP RIF
  blender/import_rif.py      headless import (+ preview render)
  blender/export_rif.py      headless export (+ validation)
  blender/start_blender_mcp.sh  Blender UI on a virtual display with the blender-mcp bridge
  game/avpctl.sh             drive the game on a virtual display (menus, console, input, screenshots)
  game/make_overlay.sh       overlay replacement files on the stock data without touching it
  samples/                   original sample models (.blend) and build_samples.sh
```

## 1. Installing the game data

The Linux port needs the installed, lower-cased data of the Gold Edition.
The retail CD stores most of it scrambled (see RESOURCES.md §1):

```sh
tools/disc/avp_install.py /path/to/AVP_GOLD.img /mnt/truenas/projects/avpmp/data   # .img/.bin/.iso or a mounted CD root
AVP_DATA=/mnt/truenas/projects/avpmp/data ./avp -w
```

Options: `--sound low` for the low-quality sound set, `--no-fmv`,
`-n` for a dry run.

## 2. Command-line tools

```sh
cd tools/rif
python3 rifdump.py /mnt/truenas/projects/avpmp/data/avp_huds/hnpcalien.rif | less          # chunk tree
python3 rifdump.py FILE.rif -i OBHIERNM,OBJHIERD,BMPNAMES             # decode selected chunks
python3 rifdump.py FILE.rif --json > tree.json
python3 rifcheck.py /mnt/truenas/projects/avpmp/data/avp_huds/*.rif                          # kind guessed from the path
python3 rifcheck.py --kind hud my_weapon.rif
python3 rifpatch.py list /mnt/truenas/projects/avpmp/data/avp_rifs/derelict.rif --placed     # placed objects
python3 rifpatch.py shape  BASE.rif NEW.rif OUT.rif --object table      # swap one object's mesh
python3 rifpatch.py hierarchy BASE.rif NEW.rif OUT.rif --name "Pulse Rifle"
python3 rifcompare.py A.rif B.rif --hierarchy alien
```

All tools read plain, Huffman-compressed and CD-scrambled RIFs. Writers emit
plain `REBINFF2` by default (`--compress` for `REBCRIF1`; the game loads
both).

Library use:

```python
from avprif import chunks, codecs, model, validate
root = chunks.load("hnpcalien.rif")
for h in model.read_hierarchies(root):
    print(h.name, [s.name for s in h.sections()], h.sequence_keys())
mesh, header = model.read_shape(model.shapes_by_id(root)[0])
chunks.save(root, "copy.rif")
```

Tests:

```sh
python3 -m unittest discover tools/rif/tests                   # synthetic
AVP_DATA=/mnt/truenas/projects/avpmp/data python3 -m unittest discover tools/rif/tests  # + every stock RIF
```

## 3. Blender add-on

Install: symlink or copy `tools/blender/io_scene_avp_rif` (it contains a
symlink to `tools/rif/avprif`; copy that directory in place when making a
zip) into Blender's `scripts/addons/`, enable *AvP RIF models*, and set the
data directory in the add-on preferences (used to find stock textures on
import).

### Conventions

| Blender | game |
|---|---|
| 1 m, Z up, model faces −Y | 1000 units (mm), y down, z forward (RESOURCES.md §3) |
| mesh object (no armature) | `RBOBJECT` + `REBSHAPE`; geometry in object space (scale applied) |
| armature | hierarchy; name from `avp_hierarchy` (else object name) |
| bone | section; bone name = section name = object name |
| mesh with Armature modifier + vertex groups | split per bone: each face goes to the bone with the largest weight |
| Action with `avp_sequence` | one sequence; also `avp_sub_sequence`, `avp_loop` (default true), `avp_time_ms` (default from the frame range; 0 = speed set by the code), `avp_aliases` (`"t:s, t:s"` exports the same animation under more ids), `avp_order` (first = default sequence) |
| Action pose markers | keyframe flags: `delta`, `sound=N`, `event=0xN`, `flags=0xN` |
| material with an Image Texture | texture written as `graphics/<folder>/<material>.bmp`, IFF flag set; materials without an image get a solid-colour 8×8 texture. Custom props `avp_transparent`, `avp_double_sided` |

Polygons with more than 4 vertices are triangulated. Make textures fairly
bright: the game's lighting is dim.

Import creates the same structure from a RIF (armature + skinned meshes +
one action per sequence, stock textures decoded from the fastfiles), so
stock models can be studied and their animations reused locally.

### Headless use

```sh
blender -b --factory-startup --python tools/blender/import_rif.py -- \
    /mnt/truenas/projects/avpmp/data/avp_huds/hnpcalien.rif --hierarchy alien --clear \
    --action 'alien|AlienRun_Standard' --frame 8 --render run.png --save alien.blend
blender -b model.blend --python tools/blender/export_rif.py -- out/avp_huds/hnpcalien.rif \
    --riffname HNPCalien --texture-dir 'open\alien' --check hierarchy
```

### Driving Blender through MCP

`tools/blender/start_blender_mcp.sh` starts Blender's UI on a virtual X
display (default `:97`) with the
[blender-mcp](https://github.com/ahujasid/blender-mcp) bridge listening on
`localhost:9876` and this add-on enabled. Register the MCP server with your
client, e.g. for Claude Code:

```sh
claude mcp add blender -e BLENDER_MCP_DISABLE_TELEMETRY=true -- uvx blender-mcp
```

Then the agent can build meshes, rigs and actions with Python, inspect
them, and call `bpy.ops.export_scene.avp_rif(...)`. The samples were made
this way.

## 4. Testing in the game

```sh
tools/game/make_overlay.sh /mnt/truenas/projects/avpmp/data /tmp/test-data my_pack/     # my_pack/avp_huds/..., graphics/...
AVP_DATA=/tmp/test-data AVP_ARGS=-d tools/game/avpctl.sh start
tools/game/avpctl.sh marine1                 # profile -> Marine episode 1
tools/game/avpctl.sh console ALIENBOT        # needs -d
tools/game/avpctl.sh grab; tools/game/avpctl.sh fire 1
tools/game/avpctl.sh shot out.png
```

`make_overlay.sh` never modifies the base directory and only deletes
overlay directories it created (marked with `.avp-overlay`). The game is
32-bit and renders through Mesa on Xvfb; see the repository `CLAUDE.md` for
building it.

## 5. Samples

`samples/crate`, `samples/pulse_rifle`, `samples/alien`: original models
(static prop, Marine HUD pulse rifle with 7 animations, alien NPC with 11
animations covering all 44 sequence ids). Rebuild them and assemble a
playable overlay:

```sh
BLENDER=blender tools/samples/build_samples.sh /tmp/avp-samples /mnt/truenas/projects/avpmp/data
AVP_DATA=/tmp/avp-samples/data AVP_ARGS=-d tools/game/avpctl.sh start
```
