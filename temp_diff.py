# -*- coding: utf-8 -*-
import pathlib, sys
sys.stdout.reconfigure(encoding='utf-8')
with open('Docs/D3D12_Textbook/02_Part1_Ch02_RenderingMath.md', encoding='utf-8') as fh:
    for i,line in enumerate(fh,1):
        if '´Ù¸¨´Ï´Ù' in line:
            print(i, line.strip())
