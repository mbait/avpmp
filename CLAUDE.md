# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Unofficial Linux port (icculus.org/avp) of Rebellion's *Aliens versus Predator* (Gold Edition) source release. The original code was written for Win32/DirectX; this port keeps most of it and swaps the platform layer for SDL 1.2 + OpenGL + OpenAL. Development history is mostly removing dead Windows/Watcom code and fixing renderer/sound issues.

## Build

```sh
make            # builds ./avp
make clean
make distclean  # also removes editor backup files
```

- Plain hand-written `Makefile`; no configure step, no linter. `-Wall` warnings are the only static check for the C/C++ code.
- Builds natively (x86-64 or i386): needs SDL 1.2 (found via `sdl-config`) and OpenAL (found via `pkg-config openal`). On Ubuntu 24.04: `libsdl1.2-dev libopenal-dev`. For a 32-bit build: `dpkg --add-architecture i386`, install `gcc-multilib g++-multilib libsdl1.2-dev:i386 libopenal-dev:i386 libgl1-mesa-dri:i386 libglx-mesa0:i386`, and run `PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig make CC="gcc -m32" CXX="g++ -m32"`. The Makefile has no header dependencies, so `make clean` after changing a header or switching architectures.
- 64-bit (LP64) rules: data read from files (RIF chunks, fastfiles, user profiles) must use `int`/`unsigned int`/`DWORD` (4 bytes), never `long`, `size_t`, `time_t` or pointers. Don't store pointers in `int`s; Rebellion's `int *` arrays that hold pointers (e.g. texture animation lists, whose first slot is a pointer-sized sequence number) must be indexed as pointer arrays. Savegames hold native structs, so they aren't portable between 32- and 64-bit builds; user profiles are.
- Modern GCC needs `-fcommon` (tentative definitions are shared across files) and `fixer.h` includes the C++ standard headers before defining its `min`/`max` macros; keep both.
- Every source file is listed explicitly in the Makefile variables (`ROOT`, `AVP`, `SUPPORT`, `AVPWIN95`, `FRONTEND`, `GADGETS`, `WIN95`, `SHAPES`), one per directory. **A new source file must be added to the matching variable**, or it won't be compiled.
- Header lookup is flat: all source dirs are on the include path (`-Isrc -Isrc/include -Isrc/win95 -Isrc/avp ...`), so headers are included by bare name.
- Game variant is selected at compile time by adding to CFLAGS: `-DALIEN_DEMO` (Alien demo data layout: `alienfastfile/`, `aenglish.txt`), `-DMARINE_DEMO`, `-DPREDATOR_DEMO`; with none set it builds for Gold Edition. These macros default to 0 in `src/avp/win95/frontend/avp_menus.h`. (The README's `-DREGULAR_EDITION` isn't referenced anywhere in the code.)
- Optimization flags are commented out in the Makefile; the default build is `-g` debug.

## Running

The binary needs the original game data (not in the repo), with **all file and directory names lowercase**. Data dir lookup (`InitGameDirectories` in `src/files.c`): `$AVP_DATA` → directory of `argv[0]` → its realpath → each `$PATH` entry → cwd. Per-user config, logs, saves go to `~/.avp/`.

```sh
AVP_DATA=/path/to/avp-gold ./avp -w     # windowed
./avp -h                                 # options: -f/-w, -s nosound, -c nocdrom, -d debug commands, -g <libGL path>
```

Install the data from the retail Gold CD (most files on it are scrambled) with `tools/disc/avp_install.py AVP_GOLD.img ~/avp-data`. Headless runs: `tools/game/avpctl.sh` (Xvfb, menu macros, console, screenshots; passes `-g libGL.so.1` by default, set `AVP_GL` to override).

OpenGL is `dlopen`ed at runtime (`load_opengl_library` in `src/main.c`, function pointers in `src/oglfunc.c`), so GL failures only show up when entering a game. FMVs (Bink/Smacker) are stubbed out.

## Architecture

The tree is the original Rebellion layout, with the port's replacement platform layer at `src/` top level:

- `src/` (top level) — **Linux port layer** plus core 3D engine files (`module.c`, `object.c`, `maths.c`, `kshape.c`, `frustum.c`, `map.c`, `morph.c`, `shpanim.c`, ...).
  - `main.c` — SDL init, video modes, input event → AvP key translation, and the main menu/game loop (replaces `avp/win95/winmain.c`).
  - `opengl.c` — the renderer; reimplements the Direct3D renderer API (`D3D_*`, `ThisFramesRenderingHasBegun`, etc.) that game code calls, in OpenGL (replaces `avp/win95/d3d_render.cpp`).
  - `openal.c` — sound platform layer behind `psndplat.h` (replaces `avp/win95/psndplat.c`).
  - `files.c` — portable file access (`OpenGameFile`, local `~/.avp` vs global data dir, lowercasing).
  - `winapi.c` — minimal Win32 API emulation (`CreateFile`, etc.) on POSIX.
  - `stubs.c` — stub definitions for Windows-only symbols the game still references (DirectX, Bink, etc.), grouped by the original file they came from.
  - `net.c` — stubbed DirectPlay; multiplayer is not functional.
  - `menus.c`, `cdplayer.c`, `fmv.c` — replacements for the Windows menu graphics, CD audio, and FMV code.
  - `fixer.h` — the portability shim included by nearly every file: Win32 types/macros, struct packing macros (`PACKED_PUSH`/`PACKED_POP`) and MSVC vs GCC differences.
- `src/include/` — core engine headers (`3dc.h`, `module.h`, `shape.h`, `prototyp.h`).
- `src/avp/` — game logic. `bh_*.c` are "behaviours" (AI/entity types: aliens, marines, predators, doors, lifts, weapons...), dispatched via strategy blocks (`stratdef.c`, `bh_types.c`). Player, weapons, HUD, particles, savegames, etc. live here too.
- `src/avp/win95/` — originally Windows-specific game glue that is still largely portable (chunk loading setup, user I/O, vision modes, network game logic in `pldnet.c`); `frontend/` is the menu system and user profiles; `gadgets/` is the in-game console/HUD text system.
- `src/avp/support/` — C++ utility classes (strings, console bindings, text rendering, refcounting).
- `src/win95/` — Rebellion's shared toolkit: the **chunk file format** loaders (`chunk.cpp`, `*chnk.cpp`) for `.rif` level files, texture loaders (`awtexld.cpp`, IFF/BMP/PNM), and the fastfile reader.

Original Windows files that are **kept in the tree but not compiled** (useful as reference for what the port reimplements): `avp/win95/{winmain.c, d3d_render.cpp, psndplat.c, directplay.c, dp_func.c, dplayext.c, pcmenus.cpp, avpreg.cpp}`, `avp/win95/frontend/avp_menugfx.cpp`, `win95/{d3_func.cpp, dd_func.cpp, di_func.cpp, win_func.cpp, cd_player.c, dxlog.c, alt_tab.cpp, videomodes.cpp}`, plus `src/winfiles.c` (an MSVC/Windows version of `files.c` from the "Windows WIP" effort; `fixer.h` has matching `_MSC_VER` branches).

## Resource tools (`tools/`, `docs/`)

- `docs/RESOURCES.md` is the reference for the model resource formats (RIF chunks, shapes, hierarchies, animation, textures, fastfiles, CD scrambling) and the names/sequences the game code expects. Update it when you learn something new about the formats.
- `tools/rif/avprif/` is a dependency-free Python 3 library for those formats; CLIs `rifdump.py`, `rifcheck.py`, `rifpatch.py`, `rifcompare.py` sit next to it. Tests: `python3 -m unittest discover tools/rif/tests` (set `AVP_DATA=~/avp-data` to also round-trip every stock RIF).
- `tools/blender/io_scene_avp_rif/` is the Blender 4.2+ import/export add-on (it symlinks `avprif`); `import_rif.py`/`export_rif.py` run it headless, `start_blender_mcp.sh` runs it under the blender-mcp bridge.
- Test modified data with `tools/game/make_overlay.sh BASE OVERLAY PACK...` (never edit `~/avp-data` in place) and `tools/game/avpctl.sh`. In windowed mode mouse buttons only work after `avpctl.sh grab`; turning is keypad 4/6. The game's lighting is dark, so brighten screenshots before judging whether something rendered.
- Tools and docs are MIT licensed (`tools/LICENSE`, `docs/LICENSE`); the game source keeps the Rebellion license. Never commit game data or anything derived from it; `tools/samples/` must contain original work only.

## Conventions

- Mixed C and C++ (`.c` compiled with gcc, `.cpp` with g++, linked with g++); C headers used from C++ rely on `extern "C"` guards — keep them when touching shared headers.
- Code style follows the surrounding original Rebellion code: tabs, Hungarian-ish/CamelCase globals, heavy use of global state (`AvP`, `Player`, `ScreenDescriptorBlock`).
- Commit messages are short, single-line, past tense ("Fixed ...", "Removed ...").
