import struct, sys, re, collections
sys.path.insert(0, '/home/awss/Dev')
from pe import PE

g = sys.argv[1]
setbuf = int(sys.argv[2], 16)
p = PE('/mnt/c/Users/awssk/AppData/Local/Temp/%s_copy.exe' % g)
secs = {s[0]: s for s in p.secs}
_, tva, tvs, tra, trs = secs[b'.text']
rdata = secs[b'.rdata']
rlo, rhi = p.base + rdata[1], p.base + rdata[1] + rdata[2]
d = p.d

def u(a):
    return struct.unpack('<I', p.at(a - p.base, 4))[0]

def cls(vt):
    try:
        col = u(vt - 4)
        td = u(col + 12)
        name = p.at(td + 8 - p.base, 80).split(b'\0')[0]
        return name.decode() if name.startswith(b'.?A') else None
    except Exception:
        return None

def owner(callrva):
    win = p.at(callrva - 0x400, 0x400)
    best = None
    for m in re.finditer(rb'\xc7[\x00\x01\x06\x07]', win, re.S):
        v = struct.unpack('<I', win[m.end():m.end() + 4])[0]
        if rlo <= v < rhi:
            c = cls(v)
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
    win = d[o - 24:o]
    idx = None
    for j in range(len(win) - 2, -1, -1):
        if win[j] == 0x6A:
            idx = win[j + 1]
            break
        if win[j] == 0x68 and j + 5 <= len(win):
            v = struct.unpack_from('<I', win, j + 1)[0]
            if v < 0x100:
                idx = v
                break
    uses[idx].append((hex(p.base + rva), owner(rva)))
for idx in sorted(uses, key=lambda x: -1 if x is None else x):
    print(idx, uses[idx])
