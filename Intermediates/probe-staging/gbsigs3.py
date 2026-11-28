import struct, sys
sys.path.insert(0, '/home/awss/Dev')
from pe import PE

GAMES = ['tmu', 'eswc', 'tmsx', 'tmo', 'tm1']
P = {g: PE('/mnt/c/Users/awssk/AppData/Local/Temp/%s_copy.exe' % g) for g in GAMES}

SIGS = {
    'StoreBuf': '50 E8 ?? ?? ?? ?? 8B 00 89 46 14 5E C2 04 00',
    'SetBufA': '8B 44 24 04 ?? ?? ?? 56 8B F1 8B 0D',
    'StartAny': '56 8B F1 F6 46 ?? 01 75 ?? 8B 4E ?? 56 E8',
    'RunJmp': '8B C1 8B 48 ?? FF 60 ??',
    'RunJmp2': '8B C1 8B 48 ?? 8B 40 ?? FF E0',
    'LoadGbxHead': '56 8B F1 C7 46 08 00 00 00 00 E8',
}
for name, sig in SIGS.items():
    print('==', name)
    for g in GAMES:
        p = P[g]
        hits = p.find(sig)
        print('  %-5s %d' % (g, len(hits)))
        for h in hits[:5]:
            print('      ', hex(p.base + h), p.at(h - 16, 56).hex(' '))
