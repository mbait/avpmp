# AvP Gold resource formats — 3D models

This document (MIT licensed, see `LICENSE`) describes the on-disk formats the Aliens versus Predator
(Gold Edition) engine in this repository uses for **3D models**: level
geometry objects, NPC and HUD-weapon hierarchies, their animations and the
texture references they carry. It is the reference for building an open
replacement resource pack; the tools in [`tools/`](../tools/README.md)
implement everything described here.

Other resource types (sounds, HUD/menu bitmaps, fonts, level logic, FMVs,
`language.txt` strings) are out of scope for this iteration and are listed
in [§15](#15-what-an-open-pack-still-needs).

How facts were established is marked where it matters:

* **[src]** read from the engine source in `src/` (file:line references);
* **[data]** checked against all 86 RIF files of the retail Gold CD (every
  chunk of every file decodes and re-encodes, see `tools/rif/tests`);
* **[game]** observed by running the Linux build with modified data.

All multi-byte values are **little-endian**. The engine is built 32-bit
(`-m32`), so `int`/`long`/pointers are 4 bytes and structures are read with
raw casts; there is no byte swapping anywhere.

---

## Contents

1. [Retail CD and installation](#1-retail-cd-and-installation)
2. [Which files hold models](#2-which-files-hold-models)
3. [Coordinate system and units](#3-coordinate-system-and-units)
4. [RIF container](#4-rif-container)
5. [Compressed RIFs (REBCRIF1)](#5-compressed-rifs-rebcrif1)
6. [Textures](#6-textures)
7. [Shapes](#7-shapes)
8. [Objects](#8-objects)
9. [Hierarchies](#9-hierarchies)
10. [Animation](#10-animation)
11. [Game contracts: names and sequences the code expects](#11-game-contracts)
12. [Loader requirements (hard failures)](#12-loader-requirements)
13. [Engine and data quirks](#13-engine-and-data-quirks)
14. [Authoring workflow](#14-authoring-workflow)
15. [What an open pack still needs](#15-what-an-open-pack-still-needs)

---

## 1. Retail CD and installation

### 1.1 Disc layout [data]

The Gold Edition CD (`AVP_GOLD`, CloneCD image) has one data track that mixes
**Mode 2 Form 1** sectors (user data at offset 24 of each 2352-byte raw
sector) and **Mode 1** sectors (user data at offset 16). A raw image must be
converted per sector by looking at the mode byte (offset 15). The ISO9660
volume has a Joliet tree with the long file names; the primary tree holds
8.3 names (`ALIEN_~1.RIF`).

The InstallShield cabinet (`data1.cab`) only *references* files stored loose
on the disc. Its file groups map to the installed layout:

| CD directory | installed as | notes |
|---|---|---|
| `avp_huds/` | `avp_huds/` | player, HUD-weapon and NPC RIFs (scrambled) |
| `avp_rifs/` | `avp_rifs/` | level RIFs (scrambled) |
| `fastfile/*.ffl`, `fastfile/ffinfo.txt` | `fastfile/` | texture archives; all but `Tex1.ffl`/`Tex37.ffl` and `ffinfo.txt` scrambled. `fastfile/1/` holds stale duplicates and is not installed |
| `HSound/fastfile/` or `LSound/fastfile/` | `fastfile/` | high/low quality sound packs (not scrambled) |
| `RSound/fastfile/` | `fastfile/` | two shared sound packs (`HSound2` group) |
| `English/` | data root | `language.txt` (a REBCRIF1 package), `default.cfg`, `readme.txt`, `credits.txt` |
| `CD Tracks.txt`, `MPConfig/`, `FMVs/` | same | |
| `shape_rifs/` | not installed | 21 plain `REBINFF2` editor files |

### 1.2 Scrambling [data]

Files in `avp_huds/`, `avp_rifs/` and `fastfile/` are scrambled byte-wise
with a key that depends only on the file offset `i`:

```
encode:  c = rol8( ((p + (i mod 71)) & 0xff) ^ 0x75, 2 )
decode:  p = ( ror8(c, 2) ^ 0x75 ) - (i mod 71)        (mod 256)
```

A scrambled RIF starts with `9C CC C4 CC 8C EC E4 35`, which decodes to
`REBCRIF1`. All 143 scrambled files decode to valid `REBCRIF1`, `RFFL` or
text. Implementation: `tools/rif/avprif/disc.py`.

### 1.3 Installed data directory [src]

`tools/disc/avp_install.py SOURCE DEST` reproduces the Windows install from
the CD root or straight from a raw/ISO image, descrambling and lowercasing.
The Linux port requires lower-case names: `OpenGameFile` (`src/files.c:45`)
tries the name as given, then a lower-cased copy, and converts `\` and `:`
to `/`. Because of an off-by-one at `src/files.c:60` the **first character**
of the relative name is neither lower-cased nor converted, which is why
`Graphics\...` texture paths only resolve through the second texture root
`graphics/` (`src/files.c:601`). Keep every installed file and directory
lower-case.

The data directory is found via `$AVP_DATA`, the executable's directory, its
realpath, `$PATH`, then the current directory (`src/files.c:591`). Per-user
configuration and profiles go to `~/.avp/`.

---

## 2. Which files hold models

[src] All model files are RIFs (§4), always opened as loose files with
`OpenGameFile(..., FILETYPE_PERM)` from the data directory; they are never
read from fastfiles. A missing or unreadable RIF ends the game with
`exit(0x111)` (`src/win95/chnkload.cpp:471-534`).

| File | Loaded by | Contents |
|---|---|---|
| `avp_rifs/<level>.rif` | `LoadRifFile` (`src/avp/win95/system.c:746`) | level modules (rooms), placed objects, textures, generators, ... |
| `avp_huds/marwep.rif` (riffname `MarineWeapons`) | `InitCharacter` (`system.c:410`) | all Marine HUD weapons as hierarchies |
| `avp_huds/pred_hud.rif` (`pred_HUD`) / `alien_hud.rif` (`alien_HUD`) | same | Predator / Alien HUD hierarchies |
| `avp_huds/marine.rif`, `predator.rif`, `alien.rif`, `multip.rif` | same | player shapes, loaded *non-hierarchically* |
| `avp_huds/hnpcmarine.rif`, `hnpcalien.rif`, `hnpcpredator.rif`, `hnpchugger.rif`, `queen.rif`, `hnpc_civvie.rif`, `hnpcpred_alien.rif`, `hnpc_xenoborg.rif`, `hnpcpretorian.rif`, `sentry.rif` | `InitNPCs` (`src/avp/win95/npcsetup.cpp:42-65, 283`) | NPC hierarchies; loaded only when the level's generators need them |
| `avp_huds/disk.rif`, `mdisk.rif` | `npcsetup.cpp:563-593` | Predator disc; `mdisk.rif` existing enables the Gold weapons |
| `avp_huds/dropship.rif`, `pred ship fury.rif`, `pred ship ob.rif` | `projload.cpp:351-372` | placed hierarchies (`PLACHIER` index 0-2) |

How a RIF is turned into game data (`copy_rif_data`, `projload.cpp:1775`):

* **Level** RIFs (`CCF_ENVIRONMENT`): `REBENVDT/BMPNAMES` defines the
  texture index space; every `REBSHAPE` associated with an object becomes
  one in-memory shape per object (`projload.cpp:1947-2120`).
* **Hierarchy** RIFs (`CCF_LOAD_AS_HIERARCHY_IF_EXISTS` and a top-level
  `OBJCHIER`): every `RBOBJECT` is converted to a shape, then each top-level
  `OBJCHIER` becomes a named hierarchy in a library keyed by
  `REBENVDT/RIFFNAME` (`projload.cpp:1339-1616`).
* **Other** RIFs: only `REBSHAPE`s with *no* associated object are copied
  (`projload.cpp:2705-2728`); they are found by name with
  `GetLoadedShapeMSL` (e.g. `"Shell"`, `"Sntrymuz"`).

---

## 3. Coordinate system and units

[src][data][game] Engine space is **right-handed, x right, y down, z
forward**, in **integer millimetres** (`GlobalScale` = 1; a RIF may scale by
`REBENVDT/ENVSDSCL`). Verified on the stock alien: head at y ≈ −1650,
feet near y = 0, tail towards −z, right leg at +x.

Mapping used by the Blender add-on (Blender: right-handed, Z up, metres,
characters facing −Y):

```
avp = A · blender · 1000,   A = | -1  0  0 |
                                |  0  0 -1 |
                                |  0 -1  0 |
```

`A` is a proper rotation and its own inverse, so rotations convert as
`A·R·A`, cross products and face winding are preserved.

**Polygon winding [data]:** the stored polygon normal equals
`normalize((v1 − v0) × (v2 − v0))` for 25 335 of the 25 511 polygons in
`hnpcalien.rif`, `marwep.rif` and `derelict.rif` (174 are degenerate, 2
reversed). Faces are front-facing when their vertices run
counter-clockwise *in Blender* (an outward Blender face normal stays
outward).

---

## 4. RIF container

### 4.1 Chunks [src][data]

A RIF is a tree of chunks:

| offset | size | field |
|---|---|---|
| 0 | 8 | identifier, ASCII, NUL-padded if shorter (`OBINTDT\0`, `MODZONE\0`) |
| 8 | 4 | `int32` size of the whole chunk **including** this 12-byte header |
| 12 | size−12 | body |

*Container* chunks (C++ `Chunk_With_Children`) have a body that is just
their child chunks back to back. The parser (`src/win95/chunk.hpp:539`) walks
children until the body is consumed; a child that would overrun its parent
is a load error. Identifiers are looked up in a registry of
`(child, parent)` pairs (`RIF_IMPLEMENT_DYNCREATE`, `chunk.cpp:544`);
anything unknown is kept as an opaque blob, so unknown chunks are harmless.

Container identifiers (some only under the parent given):

`REBINFF2` (file root), `REBSHAPE`, `SUBSHAPE`, `SHPMORPH` (in
`REBSHAPE`/`SUBSHAPE`), `SHPEXTFL`, `SHPFRAGS`, `ANIMSEQU`, `ANIMFRAM` (in
`ANIMSEQU`), `ASALTTEX`, `RBOBJECT`, `OBINTDT`, `MODULEDT`, `OBJPRJDT`,
`OBJCHIER`, `OBANSEQS`, `OBANSEQC`, `REBENVDT`, `GAMEMODE`, `LIGHTSET`,
`SPECLOBJ`, `FRAGTYPE`, `REBENUMS`, `DUMMYOBJ`, `RSPRITES`, `SPRIHEAD`,
`SPRITEPC`/`SPRITEPS`/`SPRITESA` (in `SPRIHEAD`), `AVPGENEX`, `SOUNDEXD`,
`PLACHIER`, `VIOBJECT`.

**Strings.** Most strings are NUL-terminated and padded with 1–4 NULs to a
multiple of 4: `padded = (len + 4) & ~3` ("pstr" below). A few lists pack
NUL-terminated strings back to back and pad only the end of the chunk
("packed"). The original tools left padding bytes uninitialised; writers
should zero them.

### 4.2 File root [src]

A plain file is a single `REBINFF2` chunk: the 8-byte identifier, an
`int32` that **must equal the file size**, then the top-level chunks
(`src/win95/mishchnk.cpp:350-471`). After loading, every `RBOBJECT` is
linked to the `REBSHAPE` whose `SHPHEAD1.file_id_num` equals its
`OBJHEAD1.shape_id_no` (`mishchnk.cpp:604-646`, no bounds checks).

Top-level children used by models: `RIFVERIN` (one `int32` version),
`REBENVDT` (environment: texture list, `RIFFNAME`, ...), `REBSHAPE`,
`RBOBJECT`, `OBJCHIER` (hierarchies), `INDSOUND` (keyframe sounds),
`OBHALTSH`/`HSETCOLL` (alternate shape sets), `HIDEGDIS` (LOD distances).

### 4.3 `REBENVDT` children used by models

| id | body | notes |
|---|---|---|
| `ENDTHEAD` | `int32 flags; char lock_user[16]; int32 version` | |
| `RIFFNAME` | pstr | name the hierarchy library is keyed by (§11) |
| `ENVSDSCL` | `double scale` | multiplies hierarchy offsets and object-attached shape vertices |
| `SOUNDDIR` | pstr | directory prefix for `INDSOUND` wavs |
| `BMPNAMES` | texture list, §6.1 | |
| `BMNAMVER` | `int32` | texture list version |
| `BMNAMEXT` | `int32 flags; int32 reserved[12]` | |

---

## 5. Compressed RIFs (REBCRIF1)

[src][data] The retail files are Huffman-compressed. The loader accepts both
forms, so new files may be plain `REBINFF2`; plain and compressed loads are
equivalent [game].

Header (316 bytes, `src/win95/huffman.hpp:11`):

| offset | type | field |
|---|---|---|
| 0 | char[8] | `REBCRIF1` |
| 8 | int32 | compressed size (bytes of bit stream after the header) |
| 12 | int32 | uncompressed size (the whole `REBINFF2` image including its header) |
| 16 | int32[11] | number of codes of length 1…11 |
| 60 | uint8[256] | `ByteAssignment`: symbols in code order, **index 255 gets the first (shortest) code**, going downward |
| 316 | uint32[] | bit stream |

Decoding (`huffman.cpp:398-516`): canonical codes of at most 11 bits are
assigned shortest first in bit-reversed counting order; the stream is read
LSB-first from 32-bit words through a 2048-entry table. The loader skips the
first 12 bytes of the decompressed image (the `REBINFF2` header) and does
not check its size field.

The encoder works on a 257-symbol alphabet (bytes plus terminator 256 with
count 0). The terminator has no `ByteAssignment` slot, so it **must receive
the last (longest) code**; otherwise every later symbol decodes shifted by
one. `tools/rif/avprif/huffman.py` contains a bit-exact emulation of the
decoder and a compatible encoder.

`language.txt` is also a REBCRIF1 package.

---

## 6. Textures

### 6.1 Texture lists: `BMPNAMES` / `BMPLSTST` [src][data]

`REBENVDT/BMPNAMES` defines the texture index space for a file;
`SHPEXTFL/BMPLSTST` gives a shape its own list (§7.6). Layout
(`src/win95/bmpnames.cpp:95-189`):

```
int32 header            low 16 bits: entry count; high 16: legacy version
entry × count:
  int32 flags           BMPN_* (below)
  int32 index           value polygons use in colour & 0xfff
  int32 ver_enum        version (low 20 bits) | enum id << 20
  int32 priority
  uint32 transparency   r<<22 | g<<12 | b<<2  (IFF entries: H<<16 | W)
  pstr  filename
```

Flags that matter (`bmpnames.hpp`): `0x1000000` **IFF** = the filename is a
path relative to the texture root (required on the PC — entries without it
resolve to a hard-coded network path and fail, `chnktexi.cpp:121`);
`0x800` NotInPC = skipped; `0x2` UsesTransparency; `0x100`
PriorityAndTransparencyAreValid. Stock entries use `0x100010C`; new
entries work with `0x1000100` [game].

### 6.2 Lookup order [src][game]

`CL_LoadImageOnce` (`src/win95/chnktexi.cpp:244-274`) looks for
`"Graphics\" + filename`, then `"graphics/" + filename`, first inside the
loaded fastfiles, then on disk under the data directory. **A file with the
same name inside a fastfile wins over a loose file**, so replacement
textures should use new names (the tools write them to
`graphics/<folder>/<material>.bmp`). Images are cached by name.

### 6.3 Image formats [src][data]

The decoder is chosen by magic bytes, not by extension
(`src/win95/awtexld.cpp:1690`):

* `FORM`/`LIST`/`CAT ` — IFF ILBM: palettised, uncompressed or ByteRun1,
  masking "none" or "transparent colour" (`awiffld.cpp`). All 1028 stock
  textures are `.RIM` files = IFF `LIST` with a `PROP ILBM` (`BMHD`, `CMAP`,
  private `TRAN`) and one or more `FORM ILBM` bodies.
* `BM` — uncompressed Windows BMP, 1/4/8/24 bpp (`awbmpld.cpp`).
* `P6`/`P5`/`P4` — PNM.

Images become RGBA and are uploaded with `glTexImage2D`. Power-of-two sizes
are recommended. With the transparency flag, black becomes transparent for
non-IFF images.

**Brightness [game]:** the game's lighting is dim (especially for HUD
weapons and NPCs in dark levels). Base colours around 20 % grey render as
almost black; the samples use 45–60 % base values.

### 6.4 Fastfiles (`.ffl`) [src][data]

```
char   magic[4] "RFFL";  uint32 version (0);  uint32 count;
uint32 dir_size;  uint32 data_size
directory: count × { uint32 offset; uint32 length; char name[] NUL }
           each entry padded to (12 + strlen(name)) & ~3 bytes
data:      offsets relative to the start of the data block
```

Names are full game paths with backslashes (`graphics\NPCs\Alien\Bits.RIM`)
and are matched case-insensitively. `fastfile/ffinfo.txt` maps a directory
prefix to the archive that holds it (`<prefix>;<file.ffl>`; only lines
starting with a letter count; an upper-case first letter keeps the archive
loaded). Tools: `tools/rif/avprif/ffl.py`.

---

## 7. Shapes

A `REBSHAPE` container holds one mesh. In stock files every shape has
`SHPHEAD1, SHPRAWVT, SHPVNORM, SHPPNORM, SHPPOLYS, SHPUVCRD, SHPCENTR`
(plus optional chunks below). [src] `SHPRAWVT`, `SHPPOLYS`, `SHPVNORM` and
`SHPPNORM` are dereferenced unconditionally (`chnkload.cpp:2246-2482`).

### 7.1 `SHPHEAD1` [src][data]

| off | type | field |
|---|---|---|
| 0 | int32 | flags (`0x1000` EXTERNALFILE: one shape copy shared by all its objects; `0x2000` RECENTRED; others unused by the game) |
| 4 | char[16] | lock user |
| 20 | int32 | `file_id_num` — the id objects refer to; unique, ≥ 0 |
| 24 | int32 | vertex count |
| 28 | int32 | polygon count |
| 32 | float | radius (max vertex distance from the origin) |
| 36 | int32 ×6 | max.x, min.x, max.y, min.y, max.z, min.z |
| 60 | int32 | version |
| 64 | int32 | number of associated object names |
| 68 | packed strings | object names; chunk padded to 4 |

### 7.2 Geometry chunks [src][data]

| id | body |
|---|---|
| `SHPRAWVT` | `n × int32[3]` vertices, n = (size−12)/12 |
| `SHPVNORM` | `n × float[3]` unit vertex normals (one per vertex) |
| `SHPPNORM` | `n × float[3]` unit polygon normals (one per polygon) |
| `SHPPOLYS` | `n × 36-byte` records, below |
| `SHPUVCRD` | `int32 count`, then per list `int32 nverts; nverts × float[2]` |
| `SHPCENTR` | `int32 centre[3]; float radius` |
| `SHPTEXFN` | `int32 count` + packed names (legacy; texturing uses the index in `colour`) |
| `SHPMRGDT` | `n × int32`, per polygon: −1 or index of the triangle it merges with into a quad |
| `SHPPCINF` | `int32 orig_verts; int32 n; n × int32[3]` |

Polygon record (`shpchunk.cpp:830-961`):

| off | type | field |
|---|---|---|
| 0 | int32 | engine type (§7.3) |
| 4 | int32 | polygon normal index (= polygon index in all stock data) |
| 8 | uint32 | flags (§7.4) |
| 12 | uint32 | colour (§7.5) |
| 16 | int32[5] | vertex indices, **3 or 4** used, unused slots −1; slot 5 is always −1 |

### 7.3 Engine types [src][data]

Values from `ShapeItems` (`src/include/shape.h:41`). Stock models use `7`
(`I_3dTexturedPolygon`) and `24` (`I_Gouraud3dTexturedPolygon`); a few level
polygons use `2` (`I_Polygon`). After loading, `post_process_shape`
(`projload.cpp:2924`) converts every textured type to
`I_Gouraud3dTexturedPolygon` and then the renderer uses its z-buffered
variant, so 7 and 24 behave identically. Textured types are 5, 6, 7, 21,
22, 23, 24, 25.

### 7.4 Polygon flags [src]

`iflag_*` from `shape.h:488`: `0x1` notvis, `0x2` nolight, `0x10`
transparent, `0x20` no back-face culling, `0x20000` linear, `0x400000`
tx2dor3d, `0x800000` linear_s, `0x1000000` gsort_ptest, `0x40000000` txanim
(set by the loader), `0x80000000` clockwise. The loader always ORs in
`gsort_ptest | linear_s`. Common stock values: `0x0`, `0x800000`,
`0xC00000`; `0xC00000` is used for new polygons.

### 7.5 Colour word and UVs [src][data]

For textured polygons:

```
colour = (uv_list_index << 16) | texture_index        texture_index ≤ 4095
```

`texture_index` is a `BMPNAMES` (or local `BMPLSTST`) entry index, not a
position in the list. Bits 12–15 must be 0 (the game reads `colour & 0x7fff`
and treats bit 15 as "untextured"). `uv_list_index` selects a `SHPUVCRD`
list with **one (u, v) per polygon vertex**; stock files have one list per
polygon. UVs are in **texels** of the texture (0…width, 0…height, v down)
and are **truncated to integers** on load (`chnkload.cpp:2408`); the
renderer divides by the texture size. A textured polygon whose texture is
missing or whose UV list is empty becomes an invisible polyline.

### 7.6 Optional shape chunks [src]

* `SHPEXTFL` (container: `SHPEXTFN` = `double rescale; int32 version; pstr
  file`, `BMPLSTST`, `BMNAMVER`, `BMNAMEXT`, `RIFFNAME`, `EXTOBJNM` =
  `int32 pad; pstr name`): a shape from an external file with its own
  texture list. Shapes with `SHPEXTFL` are registered by name
  (`"obj@rif"`) for `GetLoadedShapeMSL`.
* `SHPPRPRO`: a pre-processed memory image of the runtime `SHAPEHEADER`
  with 32-bit pointer fix-ups; if present it replaces the geometry chunks.
  Do not write it.
* `ANIMSEQU`/`ANIMFRAM`/`ANISEQDT`/`ANIFRADT`, `ANSHCEN2`, `ASALTTEX`:
  per-vertex animation and alternate texturings (not present in the Gold
  data). `ANISEQDT`/`ANIFRADT` = `int32[5]` + name padded with
  `(len + 6) & ~3`. A sequence without `SHPMRGDT` crashes the loader
  (`chnkload.cpp:2087`).
* `SHPMORPH`/`FRMMORPH` (`int32 flags, speed, n; n × int32[3]`): morphing;
  ignored for hierarchy sections; the reader mis-indexes frames (§13).
* `TEXTANIM`: texture flip animation (`animchnk.cpp`), 418 stock uses:
  `int32 npolys`, per polygon `poly, id, nseq, nverts, flags, identifier`,
  per sequence `speed, flags, nframes, spare[2], textures[nframes],
  uv[2·nverts·nframes]` (integer texels).
* `SHPFRAGS`, `SUBSHAPE`/`SUBSHPHD` (11 `int32`/`float` fields like
  `SHPHEAD1` without lock/names), `SHPFNAME`, `FRAGDATA`, `FRAGLOCN`:
  debris fragments.

---

## 8. Objects

`RBOBJECT` containers place shapes. `OBJHEAD1` (`obchunk.cpp:419-532`):

| off | type | field |
|---|---|---|
| 0 | int32 | flags: `0x100` base, `0x200` module (room), `0x400` placed object, `0x800/0x1000/0x2000` marine/alien/predator-only, `0x100000…0x400000` not on difficulty 1/2/3 |
| 4 | char[16] | lock user |
| 20 | int32[3] | location (mm) |
| 32 | float[4] | orientation quaternion x, y, z, w |
| 48 | int32 | index |
| 52 | int32 | version |
| 56 | int32 | `shape_id_no` = `SHPHEAD1.file_id_num` of its shape |
| 60 | pstr | name, **at most 49 characters** (copied into `char[50]`) |

Other `RBOBJECT` children: `OBINTDT/OBJNOTES` (notes text),
`SHPVTINT` (pre-lit vertex intensities: `char set[8]; int32 pad; int32 n;
int32 n×`), `MODULEDT` (room links), `OBJTRAK2`, `TRAKSOUN`, `ALTLOCAT`,
`PLOBJLIT`.

**Placed objects [src][data][game].** Placed objects use the conjugate of
the stored quaternion (`chnkload.cpp:2978`). Their shapes are centred on
the object origin: e.g. a Derelict prop at `y = 1298` with shape
`min.y = −264, max.y = 301` touches the floor at `y = 1599`. A replacement
mesh must keep its own floor contact at the same local `+y`.

**Hierarchy objects.** For hierarchy RIFs the object location and
orientation are ignored; the object name links the shape to the section of
the same name (§9) and the vertices are in that section's local frame.
Objects named `L<d>#<name>` (d = 1…9) are lower levels of detail of
`<name>`, switched at distances from `HIDEGDIS` (`int32 n` = 10,
`int32 distance[n]`) or `i × 10000` (`projload.cpp:1489-1574`).

---

## 9. Hierarchies

### 9.1 Structure [src][data]

```
OBJCHIER                      one per named hierarchy (file top level)
 ├─ OBHIERNM   pstr           hierarchy name (case-sensitive lookup)
 ├─ OBANSEQS                  sequence names (tools only)
 │   └─ OBANSEQC ×n
 │       └─ OBASEQHD          num_frames, sequence, sub-sequence, name
 └─ OBJCHIER                  EXACTLY ONE: the root section
     ├─ OBJHIERD              section name = object name
     ├─ OBANALLS              this section's keyframes for every sequence
     └─ OBJCHIER ...          child sections, recursively
```

`add_hierarchy` asserts a single root child (`projload.cpp:599`) and
**silently skips** a hierarchy whose root section has no sequences
(`projload.cpp:603-614`). Without `OBHIERNM` the name is `"Template"`.

| id | body |
|---|---|
| `OBHIERNM` | pstr |
| `OBJHIERD` | `int32 n; int32 extra[n]; pstr object_name` (must not be empty) |
| `HIERBBOX` | `int32 min[3]; int32 max[3]` (unused) |
| `OBHALTSH` | alternate shape set: `int32 num; pstr name; int32 n; n × (pstr old_object, pstr new_object); int32 flags; int32 spare[3]` |
| `HSETCOLL` | shape-set collection: `int32 num; pstr name; int32 n; int32 sets[n]; int32 type_index; int32 flags` |

A section whose name matches no `RBOBJECT` has no geometry; that is how
dummies such as `Root`, `Camera Root` and `dum flash` work.

### 9.2 Where the frames live [src][data]

Stock files store every section's frames in **`OBANALLS`** and keep only the
names in the top-level `OBANSEQS` (headers without frames). The alternative
per-sequence form (`OBANSEQC` with `OBASEQFR` frames and `OBASEQTM` time)
is read only when a section has no `OBANALLS`.

`OBANALLS` (`animobs.cpp:439-511`):

```
int32 num_sequences
per sequence:
  uint32 num_frames; int32 sequence; int32 sub_sequence; uint32 time_ms
  num_frames × { float quat[4] (x,y,z,w); int32 offset[3];
                 int32 at_frame_no; uint32 flags }            36 bytes
```

Alternative chunks: `OBASEQHD` = `uint32 num_frames; int32 sequence,
sub_sequence, n; int32 extra[n]; pstr name`; `OBASEQFR` = `float quat[4];
int32 offset[3]; int32 at_frame_no, frame_ref_no; uint32 flags; int32 n;
int32 extra[n]`; `OBASEQTM` = `uint32 ms`; `OBASEQSP` and `OBASEQFL` are
tools-only.

---

## 10. Animation

### 10.1 Keyframes [src][data]

* **Sequence id** = `(sub_sequence << 16) + sequence` (`hmodel.c:114`).
* **Time** within a sequence runs 0…65535 (`at_frame_no`). The first key
  must be at 0 and times must strictly increase.
* **Duration**: `time_ms` (from the *root* section) × 65536 / 1000. Many call
  sites pass their own duration; `time_ms = 0` means "decided by code" and
  is common for idle sequences.
* **Loop rule** (`projload.cpp:1206`): the last key's segment length is
  `65536 − at`. If it is **≤ 6** the sequence does not loop (the last key is
  held); otherwise it interpolates from the last key back to the first. So a
  cycle must *not* repeat the first pose at the end, and a one-shot puts its
  last key at 65535. The rule is evaluated per section.
* **Interpolation** (`hmodel.c:810-1069, 2525`): offsets linearly,
  orientation with slerp along the shorter arc. One-key tracks are rigid.
* **Precision**: offsets are stored as int16 (shifted `>>4` beyond ±32767);
  quaternions as int16 (≈1/32768).
* **Defaults**: a section's first sequence (`sequence_array[0]`) is used
  when a requested sequence is missing (`hmodel.c:118-141`), for gore, and
  for the HUD camera (§11.2). Give every section the same sequences in the
  same order.

### 10.2 Transform composition [src][data][game]

For each section (`hmodel.c:1233-1327`, quaternion conjugated on load and
transposed on use, which cancels out):

```
R_world(child)   = R_world(parent) · R(q_file)
pos_world(child) = pos_world(parent) + R_world(parent) · offset
vertex_world     = pos_world(section) + R_world(section) · vertex_local
```

`R(q)` is the ordinary Hamilton rotation matrix of `(x, y, z, w)`. Keys are
absolute local transforms, not deltas from a bind pose. The root's parent
is the object's world matrix. This was verified by posing the stock alien in
Blender and by a full round trip (import → Blender → export) that the game
plays identically.

### 10.3 Keyframe flags [src][data]

| bits | meaning |
|---|---|
| `0x80000000` | **DeltaFrame**: the sequence is a *delta* sequence relative to this key (at most one per section and sequence) |
| `0x7F000000` | sound index 1…127 → `INDSOUND.index` |
| `0x00FFFFFF` | game event bits, ORed into the controller's `keyframe_flags` when the key is reached |

Delta sequences are stored relative to the flagged pose at load time
(`projload.cpp:1223-1269`) and applied on top of the main animation by
`Add_Delta_Sequence`. Stock delta sequences flag *every* section (the
alien's elevation and hit sequences). **Dropping the delta flag makes the
model fall apart in-game [game]** because the full pose is then applied as
an offset.

`INDSOUND` (file top level): `int32 index; pstr wav; int32 inner, outer,
max_volume, pitch, flags; int32 n; int32 extra[n]`; the file is
`SOUNDDIR\wav`. Indices are per RIF (`marwep.rif` shares one table between
its weapons).

---

## 11. Game contracts

### 11.1 Riffnames and hierarchy names [src]

`GetNamedHierarchyFromLibrary(riffname, name)` finds hierarchies by the
RIF's `RIFFNAME` (case-insensitive) and `OBHIERNM` (**case-sensitive**). The
section damage tables in `src/avp/secstats.c` match the riffname
case-sensitively (`"HNPCalien"`, `"pred_hud"`).

| RIF | `RIFFNAME` | hierarchy names used by the code |
|---|---|---|
| `marwep.rif` | `MarineWeapons` | `Pulse Rifle`, `Smart Gun`, `Flamethrower`, `Rocket Launcher`, `Grenade Launcher`, `Minigun`, `Cudgel`, `Pistol`, `SD`, `Two pistol`, `Template` (`equipmnt.c`) |
| `pred_hud.rif` | `pred_HUD` | `Template`, `pistol`, `Speargun`, `disk`, `staff`, `wrist blade`, `wrist display` |
| `alien_hud.rif` | `alien_HUD` | `claws`, `eat` |
| `hnpcmarine.rif` | | `Template`, `marine with pulse rifle`, `marine with flame thrower`, `marine with smart gun`, `marine with SADAR`, `marine + grenade launcher`, `Marine with Mini Gun`, `PISTOL`, `Two Pistol`, `skeeter`, `Cudgel` |
| `hnpcalien.rif` | `HNPCalien` | `alien` |
| `hnpcpredator.rif` | | `Template`, `pred + pistol`, `pred with wristblade`, `pred with Plasma Caster`, `pred with staff`, `medicomp`, `Speargun`, `pred with disk` |
| others | | `hnpcpred_alien` `TEMPLATE`; `hnpcpretorian`, `hnpchugger`, `queen` `Template`; `hnpc_xenoborg` `xenobasic`; `sentry` `gun`; `disk` `Disk`; `mdisk` `Mdisk`; `hnpc_civvie` `TEMPLATE`, `male_*`, `Android *` |

### 11.2 HUD weapons [src][game]

* `Camera Root` section required (`GLOBALASSERT`, `weapons.c:3252`). The
  negated offset of its first key in `sequence_array[0]` becomes the view
  offset: the hierarchy origin is drawn at `eye + (−offset)` and then
  aimed at the crosshair target (`weapons.c:2803-2905`). The stock rifle
  uses `(−343, −312, −286)`; the sample puts `Camera Root` at
  `(−330, −400, −300)` so the rifle origin is ~0.3 m in front of the eye.
* `dum flash` (or `Dum flash` / `Dum Flash`) positions the muzzle flash.
* The template's `WeaponShapeName`/`MuzzleFlashShapeName` (`"Shell"`,
  `"Sntrymuz"` for the pulse rifle) are looked up as named shapes and must
  exist somewhere in the loaded RIFs (they are not in the hierarchy).
* Marine HUD sequences: type 10 (`HMSQT_MarineHUD`), subs `0` Stationary,
  `1` Standard_Reload, `2` Standard_Fire, `3` Come, `4` Go, `5` Fidget,
  `6` Secondary_Fire (`src/avp/sequnces.h:266`). Stock pulse-rifle
  sequences all loop with `time_ms = 0`. Swap-in starts with `Come`, so the
  weapon is briefly off-screen when a level starts.
* Other required section names: two pistols `Dum Flash L`, `Dum R Pistol
  round`, `Dum L Pistol round`; Predator `disk`, `Staff ROOT`, `Staff R
  blade`, `Staff L blade`.

### 11.3 NPCs [src][game]

* Code looks up sections by name (`GetThisSectionData`): alien `left
  thigh`, `chest`, `head`; marine `head`, `chest`, `dum flash`, `dum L
  flash`, weapon elevation sections; predator `head`, `chest`, gun
  sections; xenoborg flash dummies.
* `secstats.c` maps (riffname, hierarchy, section) to health, damage
  multipliers, gore and blood. Sections not listed get defaults that pass
  damage to the parent. Using the stock section names (`abdom`, `chest`,
  `neck`, `head`, `bite main`, `left/right thigh/shin/foot/bicep/
  forearm/palm`, `pipe_lt`, `pipe_rt`, `l00 tail` … `l11 tail`, …) makes a
  new alien take damage, bleed and die like the original [game].
* **Every sequence id the AI may request should exist**; a missing one falls
  back to `sequence_array[0]`, and a missing *delta* sequence produces a
  broken pose. The stock alien has 44 ids:

| type | subs (stock name) |
|---|---|
| 0 AlienRun | 0 Standard, 2 Attack_swipe, 3 Jump |
| 1 AlienCrawl | 0 Standard, 7 pain_fall_right, 9 boom_fall_back, 12 Attack_swipe, 13 crawl_hurt, 14 scamper |
| 2 AlienStand | 0 Standard, 1 Dies, 2 Attack_swipe, 3 Attack_bite, 4/5 pain_fall_fwd/back, 8/9 boom_fall_fwd/back, 12 spin, 14 feed, 15 taunt, **17 standard_elevation (delta, one-shot)**, 20 Attack_left_swipe_in, 21 Attack_tail, 22 Dormant, 23 Unfurl (one-shot), 24/25 Attack_both_in/down, 26 spasm (one-shot), 31/32 attack_low_left/right_swipe, 33/34 Taunt2/3, **36/37 Hit_left/right (delta)** |
| 3 AlienCrouch | 0 Standard, 1 Dies, 2 Attack_bite, 3 Attack_tail, 4 Attack_swipe, **6 standard_elevation (delta, one-shot)**, 7 pounce (one-shot), **8/9 Hit_left/right (delta)**, 10 Taunt |

The full enums are in `src/avp/sequnces.h`. NPC RIFs are loaded only if
the level has generators for them; with `-d` the console commands
`ALIENBOT`, `MARINEBOT`, `PREDOBOT`, ... spawn one in front of the player
(`src/avp/gamecmds.cpp:388`).

---

## 12. Loader requirements

Checked by `tools/rif/rifcheck.py`; violating the first group crashes,
exits or silently drops data.

1. `REBINFF2` size field equals the file size; every chunk size includes its
   12-byte header and fits its parent.
2. `REBENVDT` present; hierarchy RIFs need `RIFFNAME`.
3. Every `REBSHAPE` has `SHPHEAD1` with a unique `file_id_num ≥ 0` and
   `SHPRAWVT`, `SHPPOLYS`, `SHPVNORM` (one per vertex), `SHPPNORM`.
4. Polygons have 3 or 4 valid vertex indices; textured polygons have a
   texture index present in the list (with the IFF flag) and a UV list
   with one entry per vertex.
5. Every `RBOBJECT` has `OBJHEAD1` with an existing `shape_id_no` and a
   name of at most 49 characters.
6. Each top-level `OBJCHIER` has exactly one child `OBJCHIER`; each
   section has a non-empty `OBJHIERD` name and at least one sequence; the
   root needs sequences or the hierarchy is skipped.
7. Key times start at 0, strictly increase, ≤ 65535; at most one delta key
   per track; quaternions normalised.
8. HUD hierarchies have `Camera Root`.

---

## 13. Engine and data quirks

| where | quirk |
|---|---|
| `src/files.c:60` | first character of a relative path is not lower-cased or separator-converted |
| `shpchunk.cpp:1014` | `SHPCENTR` writer stores the shape radius, not the radius about the centre |
| `shpchunk.cpp:1900-1977` | `FRMMORPH` frames read/written with the wrong stride; only frame 0 survives |
| `chnkload.cpp:2087` | vertex-animated shapes without `SHPMRGDT` crash |
| `shpchunk.cpp` | `CONSHAPE` (console shapes) is not registered; its constructor would loop forever |
| `obchunk.cpp:159` | `file_equals` reads the object name at a stale offset (tools only) |
| `mishchnk.cpp:604` | shape array indexed by `file_id_num` without bounds or initialisation |
| `hadleyshope.rif` [data] | object name of 52 characters overflows the engine's `char[50]` |
| `invasion*.rif`, `sulaco*.rif` [data] | corrupted chunk headers (`OBIN\0\0\0\0`, `(\0…`) left by the original tools; the game skips them as unknown chunks |
| windowed play [game] | mouse buttons are only read while input is grabbed (Ctrl+G); turning is on keypad 4/6, the arrow keys strafe |

---

## 14. Authoring workflow

See [`tools/README.md`](../tools/README.md) for installation and commands.
In short:

1. Install the game data: `tools/disc/avp_install.py AVP_GOLD.img ~/avp-data`.
2. Model in Blender with the `io_scene_avp_rif` add-on (live through MCP or
   in batch with `blender -b`):
   * **static props**: mesh objects; geometry in object space; one Image
     Texture per material; export, then `rifpatch.py shape` it over a
     level object;
   * **hierarchies**: an armature (bones = sections, names per §11), meshes
     rigidly weighted to bones, one Action per sequence with
     `avp_sequence`, `avp_sub_sequence`, `avp_loop`, optional
     `avp_time_ms` and `avp_aliases`; pose markers `delta`, `sound=N`,
     `event=0xN`, `flags=0xN` set keyframe flags.
3. `rifcheck.py` the result, `rifpatch.py hierarchy` it into a shared file
   such as `marwep.rif`, or ship a complete NPC RIF.
4. Test with an overlay (`tools/game/make_overlay.sh`) and
   `tools/game/avpctl.sh` on a virtual display.

`tools/samples/` contains three original models made this way (a crate, a
HUD pulse rifle and an alien NPC) and `build_samples.sh` to rebuild them.

---

## 15. What an open pack still needs

Models are only one part of the data the game loads. A complete open pack
also needs, in rough order of importance:

* **Levels** (`avp_rifs/*.rif`): rooms (modules with `MODULEDT` links,
  `VMDARRAY`, `ADJMDLEP`), lighting (`LIGHTSET`, `SHPVTINT`), game
  objects (`SPECLOBJ`: `AVPGENER` generators, `AVPSTART` player starts,
  doors, lifts, switches, `AVPSTRAT` strategies, `PARGENER`, `SOUNDOB2`,
  `VIOBJECT` …), AI paths (`AVPPATH2`, `WAYPOINT`, `AIMODMAS`/`AIMODSLA`).
* **Player and NPC model sets** for every hierarchy in §11.1 (marines,
  predators, civilians, xenoborg, queen, ...), plus named shapes
  (`"Shell"`, muzzle flashes, debris `FRAGTYPE`s, sprites `RSPRITES`).
* **Textures** for all of the above; HUD/menu graphics (`graphics/Menus`,
  `HUDs\*.RIM`), fonts.
* **Sounds**: `fastfile/common.ffl`, `Snd*.ffl`, `*sound.dat`.
* **Text**: `language.txt` (a REBCRIF1 package), `default.cfg`.
