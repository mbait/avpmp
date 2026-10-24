#!/usr/bin/env python3
"""Check RIF files against the engine's loader requirements.

    rifcheck.py [--kind level|hierarchy|hud|shapes] FILE.rif [...]

Without --kind the kind is guessed from the path (avp_rifs/ -> level,
marwep/pred_hud/alien_hud -> hud, files with OBJCHIER -> hierarchy).
Exit status is 1 if any file has errors.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from avprif import chunks, validate  # noqa: E402

HUD_RIFS = ("marwep", "pred_hud", "alien_hud")


def guess_kind(path, root):
    base = os.path.basename(path).lower()
    if "avp_rifs" in path.replace("\\", "/").split("/"):
        return "level"
    if base.startswith(HUD_RIFS):
        return "hud"
    if root.find("OBJCHIER") is not None:
        return "hierarchy"
    return "shapes"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="+")
    ap.add_argument("--kind", choices=("level", "hierarchy", "hud", "shapes"))
    ap.add_argument("-q", "--quiet", action="store_true", help="only print errors")
    a = ap.parse_args()
    bad = 0
    for f in a.files:
        try:
            root = chunks.load(f)
        except Exception as e:
            print("%s: error: %s" % (f, e))
            bad += 1
            continue
        kind = a.kind or guess_kind(f, root)
        res = validate.check(root, kind)
        errs = [m for lvl, m in res if lvl == "error"]
        warns = [m for lvl, m in res if lvl == "warning"]
        bad += bool(errs)
        print("%s: %s, %d errors, %d warnings" % (f, kind, len(errs), len(warns)))
        for m in errs:
            print("  error:", m)
        if not a.quiet:
            for m in warns[:50]:
                print("  warning:", m)
            if len(warns) > 50:
                print("  ... %d more warnings" % (len(warns) - 50))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
