import pathlib, sys
sys.stdout.reconfigure(encoding='utf-8')
files = sorted(pathlib.Path('Docs/D3D12_Textbook').glob('*_Ch*_*.md'))
for f in files:
    prefix = f.name[:2]
    if prefix.isdigit() and 1 <= int(prefix) <= 16:
        with open(f, encoding='utf-8') as fh:
            for i,line in enumerate(fh,1):
                if 'Direct3D 12' in line:
                    print(f'{f.name}:{i}: {line.strip()}')
