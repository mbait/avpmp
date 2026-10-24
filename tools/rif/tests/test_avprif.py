"""Tests for the avprif library.

    python3 -m unittest discover tools/rif/tests

Synthetic tests need no game data. Set AVP_DATA to an installed data
directory (see tools/disc/avp_install.py) to also run the corpus tests, which
parse every stock RIF, round-trip every chunk and run the loader checks.
"""

import glob
import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from avprif import chunks, codecs, disc, ffl, huffman, image, model, validate  # noqa: E402


def cube_mesh(s=100):
    V = [[x, y, z] for x in (-s, s) for y in (-s, s) for z in (-s, s)]
    i = lambda x, y, z: (x > 0) * 4 + (y > 0) * 2 + (z > 0)  # noqa: E731
    F = [[i(-1, -1, -1), i(-1, -1, 1), i(-1, 1, 1), i(-1, 1, -1)],
         [i(1, -1, -1), i(1, 1, -1), i(1, 1, 1), i(1, -1, 1)],
         [i(-1, -1, -1), i(1, -1, -1), i(1, -1, 1), i(-1, -1, 1)],
         [i(-1, 1, -1), i(-1, 1, 1), i(1, 1, 1), i(1, 1, -1)],
         [i(-1, -1, -1), i(-1, 1, -1), i(1, 1, -1), i(1, -1, -1)],
         [i(-1, -1, 1), i(1, -1, 1), i(1, 1, 1), i(-1, 1, 1)]]
    m = model.Mesh(V)
    for f in F:
        m.polys.append(model.Polygon(f, [(0, 0), (31, 0), (31, 31), (0, 31)], 1))
    return m


def small_hierarchy():
    k0 = [model.Keyframe(0, [0, 0, 0, 1], [0, -500, 0]),
          model.Keyframe(32768, [0, 0.38268343, 0, 0.92387953], [0, -500, 0])]
    k1 = [model.Keyframe(0, [0, 0, 0, 1], [0, -200, 0])]
    root = model.Section("body", tracks=[model.Track(2, 0, k0, 1000)])
    root.children.append(model.Section("head", tracks=[model.Track(2, 0, k1, 1000)]))
    return model.Hierarchy("test", root, {(2, 0): "Stand"})


class Synthetic(unittest.TestCase):
    def test_disc_scramble_roundtrip(self):
        data = bytes(range(256)) * 3
        self.assertEqual(disc.decode(disc.encode(data)), data)
        self.assertTrue(disc.is_scrambled(disc.encode(b"REBCRIF1" + b"\0" * 56)))
        self.assertFalse(disc.is_scrambled(b"REBINFF2" + b"\0" * 56))

    def test_huffman_roundtrip(self):
        for data in (b"hello world " * 50, bytes(range(256)) * 4, b"a" * 1000,
                     os.urandom(3000)):
            pkg = huffman.compress(data)
            self.assertEqual(pkg[:8], huffman.IDENT)
            self.assertEqual(huffman.decompress(pkg), data)

    def test_chunk_tree_roundtrip(self):
        root = model.new_rif("cube", [model.texture_entry(1, "test\\a.bmp")])
        root.add(model.build_shape(cube_mesh(), 0, ["cube"]))
        root.add(model.build_object("cube", 0))
        data = root.serialize()
        self.assertEqual(struct.unpack_from("<i", data, 8)[0], len(data))
        again = chunks.load_bytes(data)
        self.assertEqual(again.serialize(), data)
        self.assertEqual(chunks.load_bytes(huffman.compress(data)).serialize(), data)
        self.assertEqual(validate.check(again, "shapes"), [])

    def test_codecs_roundtrip(self):
        root = model.new_rif("t")
        root.add(model.build_shape(cube_mesh(), 0, ["cube"]))
        root.add(model.build_object("cube", 0))
        root.add(model.build_hierarchy(small_hierarchy()))
        for _, ch in root.walk():
            if not ch.is_container and codecs.has_codec(ch.id):
                ok, msg = codecs.roundtrip_ok(ch)
                self.assertTrue(ok, msg)

    def test_normals_and_winding(self):
        m = cube_mesh()
        for p, n in zip(m.polys, m.poly_normals()):
            c = [sum(m.verts[i][k] for i in p.verts) / 4.0 for k in range(3)]
            self.assertGreater(sum(a * b for a, b in zip(c, n)), 0)  # outward

    def test_hierarchy_roundtrip_and_validation(self):
        root = model.new_rif("t")
        root.add(model.build_shape(cube_mesh(), 0, ["body"]))
        root.add(model.build_object("body", 0))
        root.add(model.build_hierarchy(small_hierarchy()))
        data = root.serialize()
        hs = model.read_hierarchies(chunks.load_bytes(data))
        self.assertEqual(len(hs), 1)
        h = hs[0]
        self.assertEqual(h.name, "test")
        self.assertEqual([s.name for s in h.sections()], ["body", "head"])
        self.assertEqual(h.seq_names[(2, 0)], "Stand")
        res = validate.check(root, "hierarchy")
        self.assertFalse([m for lvl, m in res if lvl == "error"], res)
        pose = model.pose_sections(h, (2, 0), 0)
        self.assertEqual([round(x) for x in pose["head"][0]], [0, -700, 0])

    def test_validation_catches_errors(self):
        root = model.new_rif("t")
        root.add(model.build_object("orphan", 7))
        msgs = [m for lvl, m in validate.check(root, "shapes") if lvl == "error"]
        self.assertTrue(any("missing shape" in m for m in msgs))

    def test_sample_and_reduce(self):
        t = small_hierarchy().root.tracks[0]
        q, off = model.sample_track(t, 16384)
        self.assertAlmostEqual(off[1], -500)
        dense = [model.Keyframe(at, *model.sample_track(t, at)) for at in range(0, 65536, 1024)]
        red = model.reduce_keys(dense, True)
        self.assertLess(len(red), len(dense))
        for at in range(0, 65536, 500):
            a = model.sample_track(t, at)[0]
            b = model.sample_track(model.Track(2, 0, red), at)[0]
            self.assertGreater(abs(sum(x * y for x, y in zip(a, b))), 0.9999)

    def test_ffl_roundtrip(self):
        files = [("graphics\\a.bmp", b"BM1234"), ("graphics\\dir\\b.rim", b"xyz")]
        self.assertEqual(ffl.read(ffl.write(files)), files)

    def test_bmp_roundtrip(self):
        rgba = b"".join(bytes((x * 7 % 256, y * 5 % 256, 9, 255)) for y in range(5) for x in range(6))
        w, h, out = image.read_bmp(image.write_bmp24(6, 5, rgba))
        self.assertEqual((w, h, out), (6, 5, rgba))


DATA = os.environ.get("AVP_DATA")


@unittest.skipUnless(DATA and os.path.isdir(os.path.join(DATA or "", "avp_huds")),
                     "set AVP_DATA to run corpus tests")
class Corpus(unittest.TestCase):
    def rifs(self):
        return sorted(glob.glob(os.path.join(DATA, "avp_huds", "*.rif")) +
                      glob.glob(os.path.join(DATA, "avp_rifs", "*.rif")))

    def test_every_rif(self):
        for path in self.rifs():
            with self.subTest(path=os.path.basename(path)):
                raw = open(path, "rb").read()
                plain = huffman.decompress(raw) if raw[:8] == huffman.IDENT else raw
                root = chunks.load_bytes(plain)
                self.assertEqual(root.serialize(), plain)
                for _, ch in root.walk():
                    if not ch.is_container and codecs.has_codec(ch.id):
                        ok, msg = codecs.roundtrip_ok(ch)
                        self.assertTrue(ok, msg)
                kind = "level" if "avp_rifs" in path else (
                    "hud" if os.path.basename(path).startswith(("marwep", "pred_hud", "alien_hud"))
                    else "hierarchy" if root.find("OBJCHIER") else "shapes")
                errs = [m for lvl, m in validate.check(root, kind) if lvl == "error"]
                # one stock object name overflows the engine's 50-byte buffer
                errs = [m for m in errs if "longer than 49" not in m]
                self.assertEqual(errs, [])


if __name__ == "__main__":
    unittest.main()
