"""Image helpers: IFF ILBM (.RIM textures) decoding, BMP/PNG writing.

The game's texture loader (src/win95/awtexld.cpp) picks a decoder from the
file's magic bytes, not its extension:
  * "FORM"/"LIST"/"CAT " -> IFF ILBM, palettised, ByteRun1 or uncompressed,
    masking none or transparent-colour (src/win95/awiffld.cpp, iff_ilbm.cpp)
  * "BM"                 -> uncompressed Windows BMP, 1/4/8/24 bpp (awbmpld.cpp)
  * "P6"/"P5"/"P4"/...   -> PNM (awpnmld.cpp)
The stock data uses .RIM files, which are IFF LIST/ILBM.
"""

import struct
import zlib


# ------------------------------------------------------------------ IFF

def _iff_chunks(data, start, end):
    p = start
    while p + 8 <= end:
        cid = data[p:p + 4]
        ln = struct.unpack_from(">I", data, p + 4)[0]
        yield cid, p + 8, ln
        p += 8 + ln + (ln & 1)


def _collect_ilbm(data, start, end, props, out):
    """Walk FORM/LIST/CAT/PROP structures and collect ILBM bodies."""
    for cid, p, ln in _iff_chunks(data, start, end):
        if cid in (b"FORM", b"LIST", b"CAT ", b"PROP"):
            typ = data[p:p + 4]
            if cid == b"PROP":
                for c2, p2, l2 in _iff_chunks(data, p + 4, p + ln):
                    props[c2] = data[p2:p2 + l2]
                continue
            if cid == b"FORM" and typ == b"ILBM":
                local = dict(props)
                body = None
                for c2, p2, l2 in _iff_chunks(data, p + 4, p + ln):
                    if c2 == b"BODY":
                        body = data[p2:p2 + l2]
                    else:
                        local[c2] = data[p2:p2 + l2]
                if body is not None and b"BMHD" in local and b"CMAP" in local:
                    out.append((local, body))
            else:
                _collect_ilbm(data, p + 4, p + ln, dict(props), out)


def _byterun1(src, n):
    out = bytearray()
    i = 0
    while len(out) < n and i < len(src):
        c = src[i]
        i += 1
        if c < 128:
            out += src[i:i + c + 1]
            i += c + 1
        elif c > 128:
            out += bytes([src[i]]) * (257 - c)
            i += 1
    return bytes(out), i


def decode_ilbm(data, index=0):
    """Decode an ILBM image -> (width, height, rgba bytes, top row first).

    index selects among multiple BODYs (the .RIM files hold several)."""
    out = []
    _collect_ilbm(data, 0, len(data), {}, out)
    if not out:
        raise ValueError("no ILBM body found")
    props, body = out[index]
    w, h, _, _, planes, masking, comp, _, transp = struct.unpack_from(">HHhhBBBBH", props[b"BMHD"])
    cmap = props[b"CMAP"]
    pal = [tuple(cmap[i:i + 3]) for i in range(0, len(cmap) - 2, 3)]
    rowbytes = ((w + 15) // 16) * 2
    nplanes = planes + (1 if masking == 1 else 0)
    pix = bytearray(w * h * 4)
    src = body
    pos = 0
    for y in range(h):
        idx = [0] * w
        for b in range(nplanes):
            if comp:
                row, used = _byterun1(src[pos:pos + rowbytes * 2 + 16], rowbytes)
                pos += used
            else:
                row = src[pos:pos + rowbytes]
                pos += rowbytes
            if b >= planes:
                continue
            for x in range(w):
                if row[x >> 3] >> (7 - (x & 7)) & 1:
                    idx[x] |= 1 << b
        o = y * w * 4
        for x in range(w):
            ci = idx[x]
            r, g, bl = pal[ci] if ci < len(pal) else (255, 0, 255)
            a = 0 if masking == 2 and ci == transp else 255
            pix[o:o + 4] = bytes((r, g, bl, a))
            o += 4
    return w, h, bytes(pix)


def ilbm_size(data, index=0):
    out = []
    _collect_ilbm(data, 0, len(data), {}, out)
    w, h = struct.unpack_from(">HH", out[index][0][b"BMHD"])
    return w, h


# ------------------------------------------------------------------ writers

def write_png(w, h, rgba):
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    raw = b"".join(b"\0" + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def write_bmp24(w, h, rgba):
    """Uncompressed 24-bit BMP (bottom-up), the format awbmpld.cpp accepts."""
    stride = (w * 3 + 3) & ~3
    rows = []
    for y in range(h - 1, -1, -1):
        r = bytearray()
        for x in range(w):
            o = (y * w + x) * 4
            r += bytes((rgba[o + 2], rgba[o + 1], rgba[o]))
        r += b"\0" * (stride - len(r))
        rows.append(bytes(r))
    img = b"".join(rows)
    hdr = struct.pack("<2sIHHI", b"BM", 54 + len(img), 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, w, h, 1, 24, 0, len(img), 2835, 2835, 0, 0)
    return hdr + info + img


def read_bmp(data):
    """Read an uncompressed 24/32-bit BMP -> (w, h, rgba top-down)."""
    if data[:2] != b"BM":
        raise ValueError("not a BMP")
    off = struct.unpack_from("<I", data, 10)[0]
    _, w, h, _, bpp, comp = struct.unpack_from("<IiiHHI", data, 14)
    if comp not in (0, 3) or bpp not in (24, 32):
        raise ValueError("only uncompressed 24/32-bit BMP supported")
    flip = h > 0
    h = abs(h)
    stride = (w * bpp // 8 + 3) & ~3
    out = bytearray(w * h * 4)
    for y in range(h):
        sy = h - 1 - y if flip else y
        row = data[off + sy * stride: off + sy * stride + stride]
        for x in range(w):
            b, g, r = row[x * bpp // 8: x * bpp // 8 + 3]
            out[(y * w + x) * 4:(y * w + x) * 4 + 4] = bytes((r, g, b, 255))
    return w, h, bytes(out)
