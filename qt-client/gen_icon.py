#!/usr/bin/env python3
"""Generate the hex puzzle icon as a PNG — mirrors the design in appicon.h."""
import math, sys
from PIL import Image, ImageDraw

COLORS = [
    (70,130,180),(255,160,50),(60,179,113),
    (220,80,80),(148,103,189),(64,200,200),(240,200,50),
]
OX = [0.0, 0.5, 0.5,  0.0,-0.5,-0.5, 0.0]
OY = [-0.5,-0.25,0.25,0.5, 0.25,-0.25,0.0]

def hex_pts(cx, cy, r):
    return [(cx + r*math.sin(i*math.pi/3),
             cy - r*math.cos(i*math.pi/3)) for i in range(6)]

def make_icon(size):
    img = Image.new('RGBA', (size, size), (0,0,0,0))
    d   = ImageDraw.Draw(img)
    cx, cy, R = size/2.0, size/2.0, size*0.46

    bg = hex_pts(cx, cy, R)
    d.polygon(bg, fill=(30,55,120,255))

    sr = R * 0.30
    for k in range(7):
        pts = hex_pts(cx+OX[k]*R, cy+OY[k]*R, sr)
        d.polygon(pts, fill=COLORS[k]+(255,))
        if size >= 32:
            w = max(1, int(size*0.018))
            d.line(pts+[pts[0]], fill=(20,20,20,255), width=w)

    w = max(1, int(size*0.05))
    d.line(bg+[bg[0]], fill=(15,35,90,255), width=w)
    return img

if __name__ == '__main__':
    size = int(sys.argv[1]) if len(sys.argv) > 1 else 256
    out  = sys.argv[2] if len(sys.argv) > 2 else 'DailyHexPuzzle.png'
    make_icon(size).save(out)
    print(f'Generated: {out}')
