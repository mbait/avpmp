#!/usr/bin/env python3
"""Print the chunk tree of a RIF file (plain, compressed or CD-scrambled).

    rifdump.py FILE.rif [-d] [-i ID[,ID..]] [--max-items N] [--json]

-d / --decode   decode known chunk types and print their fields
-i ID,...       only show these chunk ids (and decode them)
--json          dump the decoded tree as JSON
"""

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from avprif import chunks, codecs  # noqa: E402


def short(v, n):
    if isinstance(v, list) and len(v) > n:
        return [short(x, n) for x in v[:n]] + ["... (%d total)" % len(v)]
    if isinstance(v, list):
        return [short(x, n) for x in v]
    if isinstance(v, dict):
        return {k: short(x, n) for k, x in v.items()}
    if isinstance(v, bytes):
        return v[:60].decode("latin-1") + ("..." if len(v) > 60 else "")
    return v


def to_json(ch, n):
    d = {"id": ch.name}
    if ch.is_container:
        d["children"] = [to_json(c, n) for c in ch.children]
    elif codecs.has_codec(ch.id):
        d["data"] = short(codecs.decode(ch.id, ch.data), n)
    else:
        d["size"] = len(ch.data)
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file")
    ap.add_argument("-d", "--decode", action="store_true")
    ap.add_argument("-i", "--ids", default="")
    ap.add_argument("--max-items", type=int, default=8)
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args()

    root = chunks.load(a.file)
    if a.json:
        json.dump(to_json(root, a.max_items), sys.stdout, indent=1)
        print()
        return
    only = {chunks.norm_id(x) for x in a.ids.split(",") if x}
    for depth, ch in root.walk():
        if only and ch.id not in only:
            continue
        pad = "  " * depth
        if ch.is_container:
            print("%s%s (%d children)" % (pad, ch.name, len(ch.children)))
            continue
        line = "%s%s %d bytes" % (pad, ch.name, len(ch.data))
        if (a.decode or only) and codecs.has_codec(ch.id):
            line += " " + json.dumps(short(codecs.decode(ch.id, ch.data), a.max_items))
        print(line)


if __name__ == "__main__":
    main()
