import pathlib
files = sorted(pathlib.Path('Docs/D3D12_Textbook').glob('*_Ch*_*.md'))
for f in files:
    prefix = f.name[:2]
    if prefix.isdigit() and 1 <= int(prefix) <= 16:
        text = open(f, encoding='utf-8').read()
        if 'left-handed' in text or 'right-handed' in text or 'LH' in text or 'RH' in text:
            print(f.name)
            if 'left-handed' in text:
                print('  contains left-handed')
            if 'right-handed' in text:
                print('  contains right-handed')
            if '\nLH' in text:
                print('  contains LH (line break)')
