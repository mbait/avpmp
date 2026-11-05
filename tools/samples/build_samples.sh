#!/bin/sh
# Rebuild the sample models from their .blend sources and assemble a data
# overlay that can be played with the stock game data.
#
#   build_samples.sh OUT_DIR [AVP_DATA]
#
# OUT_DIR/pack/   replacement files laid out like the data dir:
#                   avp_rifs/derelict.rif    crate replacing "StartChair"
#                   avp_huds/marwep.rif      new "Pulse Rifle" HUD hierarchy
#                   avp_huds/hnpcalien.rif   new alien NPC
#                   graphics/open/...        their textures (24-bit BMP)
# OUT_DIR/data/   the stock data overlaid with pack/ (see make_overlay.sh)
#
# Run the result with:  AVP_DATA=OUT_DIR/data tools/game/avpctl.sh start
# Environment: BLENDER (default: blender)
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
TOOLS=$(cd "$HERE/.." && pwd)
OUT=${1:?usage: build_samples.sh OUT_DIR [AVP_DATA]}
DATA=${2:-/mnt/truenas/projects/avpmp/data}
: "${BLENDER:=blender}"
mkdir -p "$OUT/build" "$OUT/pack/avp_rifs" "$OUT/pack/avp_huds"
B="$OUT/build"
export_blend() {  # blend out.rif riffname texdir kind [extra args]
	blend=$1; rif=$2; riff=$3; tex=$4; kind=$5; shift 5
	"$BLENDER" -b "$blend" --python "$TOOLS/blender/export_rif.py" -- "$rif" \
		--riffname "$riff" --texture-dir "$tex" --graphics "$OUT/pack/graphics" \
		--check "$kind" "$@"
}
export_blend "$HERE/crate/crate.blend" "$B/crate.rif" crate 'open\crate' shapes
export_blend "$HERE/pulse_rifle/pulse_rifle.blend" "$B/pulse_rifle.rif" MarineWeapons \
	'open\pulse' hud --no-static
export_blend "$HERE/alien/alien.blend" "$OUT/pack/avp_huds/hnpcalien.rif" HNPCalien \
	'open\alien' hierarchy --no-static

python3 "$TOOLS/rif/rifpatch.py" shape "$DATA/avp_rifs/derelict.rif" "$B/crate.rif" \
	"$OUT/pack/avp_rifs/derelict.rif" --object StartChair
python3 "$TOOLS/rif/rifpatch.py" hierarchy "$DATA/avp_huds/marwep.rif" "$B/pulse_rifle.rif" \
	"$OUT/pack/avp_huds/marwep.rif" --name "Pulse Rifle"
"$TOOLS/game/make_overlay.sh" "$DATA" "$OUT/data" "$OUT/pack" >/dev/null
echo "pack:    $OUT/pack"
echo "overlay: $OUT/data"
