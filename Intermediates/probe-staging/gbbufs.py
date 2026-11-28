import struct, sys, collections
sys.path.insert(0, '/home/awss/Dev')
from pe import PE

p = PE('/mnt/c/Users/awssk/AppData/Local/Temp/tmu_copy.exe')
TARGETS = {0x4b3420: 'SetCoreBuffer', 0x4ad360: 'CtorInBuffer'}
text = [s for s in p.secs if s[0] == b'.text'][0]
_, va, vs, ra, rs = text
d = p.d
uses = collections.defaultdict(list)
for o in range(ra, ra + rs - 5):
    if d[o] != 0xE8:
        continue
    rva = o - ra + va
    tgt = p.base + rva + 5 + struct.unpack_from('<i', d, o + 1)[0]
    if tgt not in TARGETS:
        continue
    pushes = []
    k = o - 1
    window = d[o - 40:o]
    for j in range(len(window) - 1, -1, -1):
        if window[j] == 0x6A and j + 2 <= len(window):
            pushes.append(('imm8', window[j + 1], j))
        if window[j] == 0x68 and j + 5 <= len(window):
            pushes.append(('imm32', struct.unpack_from('<I', window, j + 1)[0], j))
    fn = [hex(v) for t, v, j in pushes if t == 'imm32' and 0x401000 <= v < 0xa32000]
    idx = [v for t, v, j in pushes if t == 'imm8']
    uses[TARGETS[tgt]].append((hex(p.base + rva), idx[:3], fn[:2]))
for k, v in uses.items():
    print('==', k, len(v))
    for e in v:
        print('  ', e)
