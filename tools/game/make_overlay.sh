#!/bin/sh
# Build a test data directory that overlays replacement files on top of an
# installed AvP data directory without modifying it.
#
#   make_overlay.sh BASE_DATA OVERLAY_DIR [REPLACEMENT_ROOT ...]
#
# OVERLAY_DIR gets symlinks to every file of BASE_DATA (directories are
# recreated), then every file under each REPLACEMENT_ROOT (laid out like the
# data dir, e.g. avp_huds/hnpcalien.rif, graphics/custom/foo.bmp) is copied
# over the symlinks. Re-running it refreshes the overlay.
#
# Safety: OVERLAY_DIR is only ever deleted if this script created it (it
# carries a .avp-overlay marker) and it must not be BASE_DATA, contain it,
# or be inside it. Replacement files never write through the symlinks.
set -eu
[ $# -ge 2 ] || { sed -n '2,10p' "$0"; exit 1; }
BASE=$(cd "$1" && pwd -P)
OUT=$2
shift 2
MARK=.avp-overlay

die() { echo "make_overlay: $*" >&2; exit 1; }
[ -n "$OUT" ] || die "empty OVERLAY_DIR"
[ ! -e "$OUT" ] || [ -d "$OUT" ] || die "$OUT exists and is not a directory"
# resolve without creating anything, then check before touching the disk
OUTABS=$(realpath -m -- "$OUT")
case "$OUTABS" in
/ | "$HOME" | "$(cd "$HOME" && pwd -P)") die "refusing to use $OUTABS" ;;
esac
case "$BASE/" in "$OUTABS"/*) die "OVERLAY_DIR $OUTABS contains BASE_DATA" ;; esac
case "$OUTABS/" in "$BASE"/*) die "OVERLAY_DIR $OUTABS is inside BASE_DATA" ;; esac
if [ ! -e "$OUTABS" ]; then
	mkdir -p "$OUTABS"
	: >"$OUTABS/$MARK"
fi
[ -f "$OUTABS/$MARK" ] || die "$OUTABS was not created by this script (no $MARK); not touching it"

# refresh: remove the previous overlay contents (only inside the marked dir)
find "$OUTABS" -mindepth 1 -maxdepth 1 ! -name "$MARK" -exec rm -rf -- {} +

(cd "$BASE" && find . -type d) | while read -r d; do mkdir -p "$OUTABS/$d"; done
(cd "$BASE" && find . \( -type f -o -type l \)) | while read -r f; do
	ln -s "$BASE/$f" "$OUTABS/$f"
done
for r in "$@"; do
	(cd "$r" && find . -type f) | while read -r f; do
		mkdir -p "$OUTABS/$(dirname "$f")"
		# drop the symlink first so cp cannot write through it into BASE_DATA
		rm -f -- "$OUTABS/$f"
		cp -- "$r/$f" "$OUTABS/$f"
		echo "overlay: $f"
	done
done
