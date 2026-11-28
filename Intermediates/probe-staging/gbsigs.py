import struct, sys
sys.path.insert(0, '/home/awss/Dev')
from pe import PE

GAMES = ['tmu', 'eswc', 'tmsx', 'tmo', 'tm1']
SIGS = {
    'FastCallRun': '8B C1 8B 48 ?? 8B 40 ?? FF E0',
    'FastCallCtor': '56 8B F1 E8 ?? ?? ?? ?? 8B 44 24 0C 8B 4C 24 08 89 46 ?? C7 06 ?? ?? ?? ?? 89 4E ?? 8B C6 5E C2 08 00',
    'SetCoreBuffer': '8B 44 24 04 83 F8 7F 56 8B F1 8B 0D ?? ?? ?? ?? 76 ?? 83 C0 80 83 C1 ?? 50 E8',
    'CmdStart': '56 8B F1 F6 46 ?? 01 75 ?? 8B 4E ?? 56 E8',
    'CmdStop': '8B 41 ?? A8 01 74 ?? 83 E0 FE 83 C8 02 6A 00 89 41 ?? 51 8B 49 ?? E8',
    'LoadGbx': '56 8B F1 C7 46 08 00 00 00 00 E8 ?? ?? ?? ?? 85 C0 75 06 33 C0 5E C2 04 00',
}

P = {g: PE('/mnt/c/Users/awssk/AppData/Local/Temp/%s_copy.exe' % g) for g in GAMES}

for name, sig in SIGS.items():
    print('==', name)
    for g in GAMES:
        p = P[g]
        hits = p.find(sig)
        print('  %-5s %d %s' % (g, len(hits), ' '.join(hex(p.base + h) for h in hits[:6])))
