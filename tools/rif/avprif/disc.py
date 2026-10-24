"""Decoder for the scrambled data files on the AvP Gold Edition CD.

On the retail Gold CD, files in avp_huds/, avp_rifs/ and most of fastfile/
are stored scrambled; the Windows installer descrambles them while copying.
Each byte at file offset i is transformed as

    c = rol8(((p + (i mod 71)) & 0xff) ^ 0x75, 2)

so decoding is p = ((ror8(c, 2) ^ 0x75) - (i mod 71)) & 0xff.
"""

PERIOD = 71
XOR = 0x75

_DEC = [[0] * 256 for _ in range(PERIOD)]
_ENC = [[0] * 256 for _ in range(PERIOD)]
for _k in range(PERIOD):
    for _c in range(256):
        _r = ((_c >> 2) | (_c << 6)) & 0xFF
        _p = ((_r ^ XOR) - _k) & 0xFF
        _DEC[_k][_c] = _p
        _ENC[_k][_p] = _c
_DEC_T = [bytes(t) for t in _DEC]
_ENC_T = [bytes(t) for t in _ENC]


def _apply(data, tables):
    data = bytes(data)
    out = bytearray(len(data))
    for k in range(PERIOD):
        out[k::PERIOD] = data[k::PERIOD].translate(tables[k])
    return bytes(out)


def decode(data):
    return _apply(data, _DEC_T)


def encode(data):
    return _apply(data, _ENC_T)


KNOWN_MAGICS = (b"REBCRIF1", b"REBINFF2", b"RFFL")


def _texty(b):
    return all(32 <= c < 127 or c in (9, 10, 13) for c in b)


def is_scrambled(data):
    """True if data does not start with a known magic but decodes to one
    (or, for text files such as fastfile/ffinfo.txt, to plain text)."""
    head = bytes(data[:64])
    if any(head.startswith(m) for m in KNOWN_MAGICS) or _texty(head):
        return False
    dec = decode(head)
    return any(dec.startswith(m) for m in KNOWN_MAGICS) or _texty(dec)
