#!/usr/bin/env python3
"""Patch content from one RIF into another.

  rifpatch.py hierarchy BASE.rif NEW.rif OUT.rif --name NAME [--keep-old-objects]
      Replace hierarchy NAME in BASE with hierarchy NAME from NEW (added if
      BASE has none). The objects/shapes of NEW's sections are copied with
      fresh shape ids; objects that only the old hierarchy used are removed.
      Textures are merged into BASE's BMPNAMES with new indices.

  rifpatch.py shape BASE.rif NEW.rif OUT.rif --object NAME [--from NAME]
      Replace the geometry of object NAME in BASE with the shape of object
      FROM (default: the only/first object) in NEW. The object keeps its
      placement; textures are merged as above.

  rifpatch.py list BASE.rif [--placed]
      List objects (name, shape id, location, polygon count).

Options: --compress writes REBCRIF1. The result is validated (rifcheck).
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from avprif import chunks, codecs, model, validate  # noqa: E402
from avprif.chunks import Chunk  # noqa: E402


def dec(ch):
    return codecs.decode_chunk(ch)


def _obj_name(o):
    return dec(o.find("OBJHEAD1"))["name"]


def _hier_objects(top):
    names = set()
    stack = [top]
    while stack:
        n = stack.pop()
        hd = n.find("OBJHIERD")
        if hd is not None:
            names.add(dec(hd)["object_name"])
        stack += n.findall("OBJCHIER")
    return names


def _hier_name(top):
    nm = top.find("OBHIERNM")
    return dec(nm)["name"] if nm is not None else "Template"


def merge_textures(base, new_root, shape_chunks):
    """Append NEW's textures to BASE's list; remap polygon texture indices in
    the given shape chunks. Returns {old_index: new_index}."""
    btex = model.texture_list(base)
    used = {e["index"] for e in btex}
    nxt = (max(used) + 1) if used else 1
    by_name = {e["filename"].lower(): e["index"] for e in btex}
    remap = {}
    for e in model.texture_list(new_root):
        key = e["filename"].lower()
        if key in by_name:
            remap[e["index"]] = by_name[key]
            continue
        if nxt > 0xFFF:
            raise SystemExit("texture index space (4096) exhausted")
        ne = dict(e)
        ne["index"] = nxt
        btex.append(ne)
        by_name[key] = nxt
        remap[e["index"]] = nxt
        nxt += 1
    model.set_texture_list(base, btex)
    for sh in shape_chunks:
        pc = sh.find("SHPPOLYS")
        d = dec(pc)
        for p in d["polys"]:
            if p["engine_type"] in validate.TEXTURED:
                t = p["colour"] & 0xFFF
                if t in remap:
                    p["colour"] = (p["colour"] & ~0xFFF) | remap[t]
        pc.data = codecs.encode("SHPPOLYS", d)
    return remap


def _set_shape_id(sh, sid, names):
    h = sh.find("SHPHEAD1")
    d = dec(h)
    d["file_id_num"] = sid
    d["object_names"] = names
    h.data = codecs.encode("SHPHEAD1", d)


def _set_object_shape(o, sid):
    h = o.find("OBJHEAD1")
    d = dec(h)
    d["shape_id_no"] = sid
    h.data = codecs.encode("OBJHEAD1", d)


def _clone(ch):
    if ch.is_container:
        return Chunk(ch.id, children=[_clone(c) for c in ch.children])
    return Chunk(ch.id, bytes(ch.data))


def cmd_hierarchy(a):
    base = chunks.load(a.base)
    new = chunks.load(a.new)
    ntops = [t for t in new.findall("OBJCHIER") if _hier_name(t) == a.name]
    if not ntops:
        raise SystemExit("NEW has no hierarchy %r" % a.name)
    ntop = ntops[0]
    btops = [t for t in base.findall("OBJCHIER") if _hier_name(t) == a.name]
    # objects referenced by other hierarchies in BASE stay
    other = set()
    for t in base.findall("OBJCHIER"):
        if t not in btops:
            other |= _hier_objects(t)
    old_objs = set()
    for t in btops:
        old_objs |= _hier_objects(t)
    insert_at = base.children.index(btops[0]) if btops else len(base.children)
    for t in btops:
        base.remove(t)
    bobjs = model.objects_by_name(base)
    bshapes = model.shapes_by_id(base)
    if not a.keep_old_objects:
        for n in sorted(old_objs - other):
            o = bobjs.get(n)
            if o is None:
                continue
            sid = dec(o.find("OBJHEAD1"))["shape_id_no"]
            base.remove(o)
            sh = bshapes.get(sid)
            still = [x for x in base.findall("RBOBJECT")
                     if dec(x.find("OBJHEAD1"))["shape_id_no"] == sid]
            if sh is not None and not still:
                base.remove(sh)
        bobjs = model.objects_by_name(base)
    # copy NEW's objects for the new sections
    nobjs = model.objects_by_name(new)
    nshapes = model.shapes_by_id(new)
    sid = model.next_shape_id(base)
    copied = []
    for n in sorted(_hier_objects(ntop)):
        o = nobjs.get(n)
        if o is None:
            continue
        if n in bobjs:
            if n in other:
                print("warning: object %r is also used by another hierarchy; it is replaced"
                      % n)
            old = bobjs[n]
            base.remove(old)
        sh = _clone(nshapes[dec(o.find("OBJHEAD1"))["shape_id_no"]])
        ob = _clone(o)
        _set_shape_id(sh, sid, [n])
        _set_object_shape(ob, sid)
        sid += 1
        base.add(sh, insert_at)
        base.add(ob, insert_at + 1)
        insert_at += 2
        copied.append(sh)
    merge_textures(base, new, copied)
    base.add(_clone(ntop), insert_at)
    return base, "hierarchy"


def cmd_shape(a):
    base = chunks.load(a.base)
    new = chunks.load(a.new)
    bobjs = model.objects_by_name(base)
    if a.object not in bobjs:
        raise SystemExit("BASE has no object %r" % a.object)
    nobjs = model.objects_by_name(new)
    src = nobjs.get(a.source) if a.source else next(iter(nobjs.values()))
    if src is None:
        raise SystemExit("NEW has no object %r" % a.source)
    nsh = model.shapes_by_id(new)[dec(src.find("OBJHEAD1"))["shape_id_no"]]
    o = bobjs[a.object]
    sid = dec(o.find("OBJHEAD1"))["shape_id_no"]
    bsh = model.shapes_by_id(base)
    sharing = [x for x in base.findall("RBOBJECT")
               if dec(x.find("OBJHEAD1"))["shape_id_no"] == sid]
    sh = _clone(nsh)
    if len(sharing) > 1:  # give this object its own shape
        newsid = model.next_shape_id(base)
        _set_shape_id(sh, newsid, [a.object])
        _set_object_shape(o, newsid)
        base.add(sh, base.children.index(o))
    else:
        old = bsh[sid]
        hd = dec(old.find("SHPHEAD1"))
        _set_shape_id(sh, sid, hd["object_names"])
        # keep shape-level extras of the old shape that are not geometry
        base.children[base.children.index(old)] = sh
        sh.parent = base
    merge_textures(base, new, [sh])
    return base, "level" if "avp_rifs" in a.base else "shapes"


def cmd_list(a):
    base = chunks.load(a.base)
    shapes = model.shapes_by_id(base)
    for o in base.findall("RBOBJECT"):
        d = dec(o.find("OBJHEAD1"))
        if a.placed and not d["flags"] & 0x400:
            continue
        sh = shapes.get(d["shape_id_no"])
        npol = dec(sh.find("SHPHEAD1"))["num_polys"] if sh else -1
        print("%-40s shape %5d  flags %#8x  loc %s  polys %d" % (
            d["name"], d["shape_id_no"], d["flags"], d["location"], npol))
    return None, None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    h = sub.add_parser("hierarchy")
    h.add_argument("base")
    h.add_argument("new")
    h.add_argument("out")
    h.add_argument("--name", required=True)
    h.add_argument("--keep-old-objects", action="store_true")
    h.add_argument("--compress", action="store_true")
    s = sub.add_parser("shape")
    s.add_argument("base")
    s.add_argument("new")
    s.add_argument("out")
    s.add_argument("--object", required=True)
    s.add_argument("--from", dest="source")
    s.add_argument("--compress", action="store_true")
    li = sub.add_parser("list")
    li.add_argument("base")
    li.add_argument("--placed", action="store_true")
    a = ap.parse_args()
    root, kind = {"hierarchy": cmd_hierarchy, "shape": cmd_shape, "list": cmd_list}[a.cmd](a)
    if root is None:
        return
    errs = [m for lvl, m in validate.check(root, kind) if lvl == "error"]
    for m in errs:
        print("error:", m)
    chunks.save(root, a.out, compress=a.compress)
    print("wrote %s (%d errors)" % (a.out, len(errs)))
    sys.exit(1 if errs else 0)


if __name__ == "__main__":
    main()
