import struct, sys, collections
sys.path.insert(0, '/home/awss/Dev')
from pe import PE

g = sys.argv[1] if len(sys.argv) > 1 else 'eswc'
target = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x4915b0
p = PE('/mnt/c/Users/awssk/AppData/Local/Temp/%s_copy.exe' % g)
text = [s for s in p.secs if s[0] == b'.text'][0]
_, va, vs, ra, rs = text
d = p.d
sites = []
for o in range(ra, ra + rs - 5):
    if d[o] != 0xE8:
        continue
    rva = o - ra + va
    if p.base + rva + 5 + struct.unpack_from('<i', d, o + 1)[0] == target:
        sites.append(rva)
print(len(sites), 'call sites')
for rva in sites[:int(sys.argv[3]) if len(sys.argv) > 3 else 8]:
    print(hex(p.base + rva))
    print('   before', p.at(rva - 24, 24).hex(' '))
    print('   after ', p.at(rva + 5, 40).hex(' '))
