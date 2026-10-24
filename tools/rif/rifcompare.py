#!/usr/bin/env python3
"""Compare two hierarchy RIFs numerically (geometry and animation).

    rifcompare.py A.rif B.rif [--hierarchy NAME] [--samples N]

For every section present in both: compares the section-local vertex sets
(nearest-vertex distance, mm) and, for every common sequence, the world
pivot positions and orientations at N evenly spaced times (keyframes
interpolated linearly / slerp like the engine). Prints maxima.
"""

import argparse
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from avprif import chunks, codecs, model  # noqa: E402


def slerp(a, b, t):
    d = sum(x * y for x, y in zip(a, b))
    if d < 0:
        b = [-x for x in b]
        d = -d
    if d > 0.9995:
        r = [x + (y - x) * t for x, y in zip(a, b)]
    else:
        th = math.acos(d)
        s = math.sin(th)
        r = [(math.sin((1 - t) * th) * x + math.sin(t * th) * y) / s for x, y in zip(a, b)]
    n = math.sqrt(sum(x * x for x in r))
    return [x / n for x in r]


def sample(track, at):
    fr = track.frames
    loop = (65536 - fr[-1].at) > 6
    for i, f in enumerate(fr):
        nxt = fr[i + 1] if i + 1 < len(fr) else (fr[0] if loop else f)
        end = fr[i + 1].at if i + 1 < len(fr) else 65536
        if f.at <= at < end or i == len(fr) - 1:
            t = 0.0 if end == f.at or nxt is f else min(1.0, (at - f.at) / float(end - f.at))
            q = slerp(f.quat, nxt.quat, t)
            o = [a + (b - a) * t for a, b in zip(f.offset, nxt.offset)]
            return q, o
    return fr[0].quat, fr[0].offset


def world(h, key, at):
    out = {}

    def rec(sec, pw, pa):
        t = sec.track(key) or sec.tracks[0]
        q, off = model.sample_track(t, at)
        w = [pw[i] + sum(pa[i][k] * off[k] for k in range(3)) for i in range(3)]
        r = model.quat_to_mat(q)
        a = [[sum(pa[i][k] * r[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
        out[sec.name] = (w, a)
        for c in sec.children:
            rec(c, w, a)
    rec(h.root, [0, 0, 0], [[1, 0, 0], [0, 1, 0], [0, 0, 1]])
    return out


def section_meshes(root):
    objs = model.objects_by_name(root)
    shapes = model.shapes_by_id(root)
    out = {}
    for n, o in objs.items():
        sid = codecs.decode_chunk(o.find("OBJHEAD1"))["shape_id_no"]
        if sid in shapes:
            out[n] = model.read_shape(shapes[sid])[0]
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--hierarchy")
    ap.add_argument("--samples", type=int, default=16)
    a = ap.parse_args()
    ra, rb = chunks.load(a.a), chunks.load(a.b)
    ha = {h.name: h for h in model.read_hierarchies(ra) if h.root.tracks}
    hb = {h.name: h for h in model.read_hierarchies(rb) if h.root.tracks}
    names = [a.hierarchy] if a.hierarchy else sorted(set(ha) & set(hb))
    ma, mb = section_meshes(ra), section_meshes(rb)
    worst_geo = 0.0
    for n in sorted(set(ma) & set(mb)):
        va, vb = ma[n].verts, mb[n].verts
        if not va or not vb:
            continue
        d = max(min(math.dist(p, q) for q in vb) for p in va)
        worst_geo = max(worst_geo, d)
        if len(ma[n].polys) != len(mb[n].polys):
            print("section %r: %d vs %d polygons" % (n, len(ma[n].polys), len(mb[n].polys)))
    print("geometry: %d common sections, max vertex distance %.2f mm"
          % (len(set(ma) & set(mb)), worst_geo))
    for hn in names:
        h1, h2 = ha[hn], hb[hn]
        k1, k2 = h1.sequence_keys(), h2.sequence_keys()
        print("hierarchy %r: %d/%d sequences, %d/%d sections" % (
            hn, len(k1), len(k2), len(h1.sections()), len(h2.sections())))
        wp = wr = 0.0
        where = None
        for key in sorted(set(k1) & set(k2)):
            for i in range(a.samples):
                at = int(i * 65535 / max(1, a.samples - 1))
                w1, w2 = world(h1, key, at), world(h2, key, at)
                for s in set(w1) & set(w2):
                    d = math.dist(w1[s][0], w2[s][0])
                    if d > wp:
                        wp, where = d, (key, at, s)
                    wr = max(wr, max(abs(w1[s][1][i][j] - w2[s][1][i][j])
                                     for i in range(3) for j in range(3)))
            t1, t2 = h1.root.track(key), h2.root.track(key)
            if abs(t1.time_ms - t2.time_ms) > 40:
                print("  seq %s time %d vs %d ms" % (key, t1.time_ms, t2.time_ms))
        print("  max pivot distance %.2f mm (at %s), max rotation matrix delta %.4f"
              % (wp, where, wr))


if __name__ == "__main__":
    main()
