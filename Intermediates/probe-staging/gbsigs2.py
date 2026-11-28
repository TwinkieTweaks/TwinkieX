import struct, sys
sys.path.insert(0, '/home/awss/Dev')
from pe import PE

GAMES = ['tmu', 'eswc', 'tmsx', 'tmo', 'tm1']
P = {g: PE('/mnt/c/Users/awssk/AppData/Local/Temp/%s_copy.exe' % g) for g in GAMES}
CTOR = '56 8B F1 E8 ?? ?? ?? ?? 8B 44 24 0C 8B 4C 24 08 89 46 ?? C7 06 ?? ?? ?? ?? 89 4E ?? 8B C6 5E C2 08 00'
CTOR_TMO = '56 8B F1 E8 ?? ?? ?? ?? 8B 44 24 0C 8B 4C 24 08'

def u32(p, rva): return struct.unpack('<I', p.at(rva, 4))[0]

for g in GAMES:
    p = P[g]
    hits = p.find(CTOR) or p.find(CTOR_TMO)
    print('==', g, [hex(p.base + h) for h in hits[:4]])
    for h in hits[:1]:
        print('   ctor bytes', p.at(h, 0x30).hex(' '))
        if g != 'tmo':
            vt = u32(p, h + 21)
            print('   vtable', hex(vt), 'fnoff', p.at(h + 18, 1).hex(), 'nodoff', p.at(h + 27, 1).hex())
            for slot in (32, 33, 34):
                fn = u32(p, vt - p.base + slot * 4)
                print('   slot', slot, hex(fn), p.at(fn - p.base, 0x18).hex(' '))
