import struct, sys, re, collections
sys.path.insert(0, '/home/awss/Dev')
from pe import PE

g = sys.argv[1]
setbuf = int(sys.argv[2], 16)
p = PE('/mnt/c/Users/awssk/AppData/Local/Temp/%s_copy.exe' % g)
code = p.secs[0]
_, tva, tvs, tra, trs = code
d = p.d
lo, hi = p.base, p.base + max(va + vs for _, va, vs, _, _ in p.secs)

def u(a):
    b = p.at(a - p.base, 4)
    return struct.unpack('<I', b)[0] if b and len(b) == 4 else None

def ret_imm(fn):
    b = p.at(fn - p.base, 6) if fn and lo <= fn < hi else None
    if b and len(b) == 6 and b[0] == 0xB8 and b[5] == 0xC3:
        return struct.unpack('<I', b[1:5])[0]
    return None

def cstr(a):
    if not a or not (lo <= a < hi):
        return None
    b = p.at(a - p.base, 64)
    if not b:
        return None
    s = b.split(b'\0')[0]
    if len(s) >= 3 and all(32 < c < 127 for c in s) and s[:1] == b'C':
        return s.decode()
    return None

def class_of(vt):
    cid = ret_imm(u(vt + 8))
    ci = ret_imm(u(vt + 4))
    name = None
    if ci:
        for off in range(0, 0x20, 4):
            name = cstr(u(ci + off))
            if name:
                break
    if cid is None and name is None:
        return None
    return (hex(cid) if cid else None, name)

def owner(callrva):
    win = p.at(callrva - 0x500, 0x500)
    best = None
    for m in re.finditer(rb'\xc7[\x00\x01\x06\x07]', win, re.S):
        v = struct.unpack('<I', win[m.end():m.end() + 4])[0]
        if lo <= v < hi:
            c = class_of(v)
            if c:
                best = c
    return best

uses = collections.defaultdict(list)
for o in range(tra, tra + trs - 5):
    if d[o] != 0xE8:
        continue
    rva = o - tra + tva
    if p.base + rva + 5 + struct.unpack_from('<i', d, o + 1)[0] != setbuf:
        continue
    win = d[o - 16:o]
    idx = None
    for j in range(len(win) - 2, -1, -1):
        if win[j] == 0x6A:
            idx = win[j + 1]
            break
    uses[idx].append((hex(p.base + rva), owner(rva)))
for idx in sorted(uses, key=lambda x: -1 if x is None else x):
    if idx is not None and idx >= 0x80:
        continue
    print(idx, uses[idx])
