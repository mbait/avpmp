"""REBCRIF1 Huffman package (de)compression.

Port of src/win95/huffman.cpp. The decoder is a literal emulation of the
original (including x86 shift-count masking) so that it reproduces the game
bit-for-bit; the encoder follows HuffmanCompression() closely enough to
produce packages the original decoder accepts.

Package layout (little-endian, 316-byte header):
    char  Identifier[8]          "REBCRIF1"
    int32 CompressedDataSize     bytes of bit stream that follow the header
    int32 UncompressedDataSize
    int32 CodelengthCount[11]    number of codes of length 1..11
    uint8 ByteAssignment[256]    symbols, least frequent first
    uint32 bitstream[]
"""

import struct

IDENT = b"REBCRIF1"
MAX_DEPTH = 11
HEADER_SIZE = 8 + 4 + 4 + 4 * MAX_DEPTH + 256
M32 = 0xFFFFFFFF


def _shl(x, n):
    return (x << (n & 31)) & M32


def _shr(x, n):
    return (x & M32) >> (n & 31)


def _make_decode_table(depth, lst):
    table = [0] * (1 << MAX_DEPTH)
    lenbits = 0
    repcount = 1 << MAX_DEPTH
    repspace = 1
    depthbit = 4
    o = 0
    p = 255
    di = 0
    while True:
        while True:
            lenbits += 1
            depthbit <<= 1
            repspace <<= 1
            repcount >>= 1
            thisdepth = depth[di]
            di += 1
            if thisdepth:
                break
        while True:
            if p < 0:
                temp = 0xFF
            else:
                temp = lenbits | (lst[p] << 8)
                p -= 1
            idx = o >> 2
            for _ in range(repcount):
                table[idx] = temp
                idx += repspace
            temp = depthbit
            while True:
                temp >>= 1
                if temp & 3:
                    return table
                o ^= temp
                if o & temp:
                    break
            thisdepth -= 1
            if not thisdepth:
                break


def decompress(data):
    """Decompress a REBCRIF1 package; returns the uncompressed bytes."""
    if data[:8] != IDENT:
        raise ValueError("not a REBCRIF1 package")
    csize, usize = struct.unpack_from("<ii", data, 8)
    depth = struct.unpack_from("<%di" % MAX_DEPTH, data, 16)
    lst = data[16 + 4 * MAX_DEPTH:HEADER_SIZE]
    table = _make_decode_table(depth, lst)
    # the original reads whole words and may run past the stream end; pad it
    stream = data[HEADER_SIZE:HEADER_SIZE + csize] + b"\0" * 16
    nwords = len(stream) // 4
    source = struct.unpack_from("<%dI" % nwords, stream)
    # table viewed as shorts: index (bits & 0xffe) -> int index (bits>>1)&0x7ff
    lens = [t & 0xFF for t in table]
    syms = [(t >> 8) & 0xFF for t in table]

    out = bytearray()
    si = 0
    available = reserve = wid = 0
    bits = resbits = 0
    length = usize
    while True:
        available += wid
        fill = 31 - available
        bits = _shl(bits, fill)
        if fill > reserve:
            fill -= reserve
            available += reserve
            if reserve:
                bits = _shr(bits, reserve) | _shl(resbits, 32 - reserve)
            resbits = source[si]
            si += 1
            reserve = 32
        bits = _shr(bits, fill) | _shl(resbits, 32 - fill)
        resbits = _shr(resbits, fill)
        reserve -= fill
        available = 31
        ti = (bits >> 1) & 0x7FF
        while True:
            wid = lens[ti]
            available -= wid
            if available < 0 or len(out) == length:
                break
            bits = _shr(bits, wid)
            out.append(syms[ti])
            ti = (bits >> 1) & 0x7FF
        if not (available > -32 and len(out) != length):
            break
    return bytes(out)


# ---------------------------------------------------------------- encoder

def _code_lengths(freq):
    """Length-limited (<= MAX_DEPTH) Huffman code lengths via package-merge."""
    syms = [s for s in range(len(freq)) if freq[s]]
    if len(syms) == 1:
        return {syms[0]: 1}
    items = sorted((freq[s], (s,)) for s in syms)
    packages = list(items)
    for _ in range(MAX_DEPTH - 1):
        merged = []
        for i in range(0, len(packages) - 1, 2):
            merged.append((packages[i][0] + packages[i + 1][0],
                           packages[i][1] + packages[i + 1][1]))
        packages = sorted(items + merged, key=lambda x: x[0])
    lengths = {s: 0 for s in syms}
    for _, members in packages[:2 * len(syms) - 2]:
        for s in members:
            lengths[s] += 1
    return lengths


def compress(data):
    """Build a REBCRIF1 package that the game's decoder accepts.

    Alphabet is the 256 byte values plus terminator symbol 0x100, as in the
    original encoder. Canonical codes are assigned in the order the decoder's
    table builder expects: shortest codes first, symbols taken from
    ByteAssignment[255] downwards, bit-reversed counting.
    """
    freq = [0] * 257
    for b in data:
        freq[b] += 1
    # the terminator has no ByteAssignment slot, so it must get the very last
    # (longest) code; the original gives it count 0, we make it the strictly
    # least frequent symbol
    freq[256] = 0.5
    lengths = _code_lengths(freq)
    # decoder assigns codes from ByteAssignment[255] downward, shortest first
    order = sorted(lengths, key=lambda s: (lengths[s], s == 256, -freq[s], s))
    if order[-1] != 256:
        raise AssertionError("terminator must take the last code")
    counts = [0] * MAX_DEPTH
    for s in order:
        counts[lengths[s] - 1] += 1
    # ByteAssignment holds byte symbols; index 255 is the first code.
    byte_order = [s for s in order if s < 256]
    unused = [s for s in range(256) if s not in lengths]
    assign = [0] * 256
    seq = byte_order + unused
    for k, s in enumerate(seq):
        assign[255 - k] = s
    # canonical, bit-reversed codes in table order (matches decoder walk)
    codes = {}
    code = 0
    prev_len = 0
    for s in order:
        L = lengths[s]
        code <<= (L - prev_len)
        prev_len = L
        rev = int(format(code, "0%db" % L)[::-1], 2)
        codes[s] = (rev, L)
        code += 1
    words = []
    acc = 0
    nbits = 0
    for b in list(data) + [256]:
        c, L = codes[b]
        acc |= c << nbits
        nbits += L
        while nbits >= 32:
            words.append(acc & M32)
            acc >>= 32
            nbits -= 32
    words.append(acc & M32)
    words.append(0)
    stream = struct.pack("<%dI" % len(words), *words)
    hdr = IDENT + struct.pack("<ii", len(stream), len(data))
    hdr += struct.pack("<%di" % MAX_DEPTH, *counts) + bytes(assign)
    return hdr + stream
