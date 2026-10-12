#!/usr/bin/env python3
"""Pre-render 9-patch drop shadows used by the compositor (BorderImage).

shadow-window.png : 160x160, a 80x80 rounded rect (r=10) centred, blurred 16px.
shadow-popup.png  : 120x120, a 56x56 rounded rect (r=12) centred, blurred 10px.
In QML the BorderImage uses margins = (size - rect)/2 and borders = margin + radius + 2.
"""
import os
from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))

def make(name, size, rect, radius, blur, alpha):
    img = Image.new("L", (size, size), 0)
    m = (size - rect) // 2
    ImageDraw.Draw(img).rounded_rectangle([m, m, m + rect - 1, m + rect - 1], radius=radius, fill=int(255 * alpha))
    img = img.filter(ImageFilter.GaussianBlur(blur))
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.putalpha(img)
    out.save(os.path.join(HERE, name))

make("shadow-window.png", 160, 80, 10, 16, 0.42)
make("shadow-popup.png", 120, 56, 12, 10, 0.30)
print("shadows written")
