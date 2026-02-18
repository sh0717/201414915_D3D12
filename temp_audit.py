import pathlib
import sys
sys.stdout.reconfigure(encoding='utf-8')
files = sorted(pathlib.Path('Docs/D3D12_Textbook').glob('*_Ch*_*.md'))
for f in files:
    prefix = f.name[:2]
    if not prefix.isdigit():
        continue
    num = int(prefix)
    if 1 <= num <= 16:
        print('---', f.name)
        with open(f, encoding='utf-8') as fh:
            for i,line in enumerate(fh,1):
                if 'D3D12' in line or 'DirectX 12' in line or 'coordinate' in line:
                    print(f'{i}: {line.strip()}')
