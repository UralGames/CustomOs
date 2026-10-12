#!/usr/bin/env python3
"""Generate Meridian's default wallpapers as SVG (rendered to JPEG at build time).

All artwork is procedural and original to this project. Run:
    python3 generate_wallpapers.py            # writes *.svg next to this script
    rsvg-convert -w 2560 -h 1440 ridge.svg | convert - ridge.jpg   (optional)
"""
import math
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 2560, 1440


def smooth_path(points, close_to_bottom=True):
    """Catmull-Rom spline through points, converted to cubic Beziers."""
    d = [f"M{points[0][0]:.1f},{points[0][1]:.1f}"]
    for i in range(len(points) - 1):
        p0 = points[i - 1] if i > 0 else points[i]
        p1, p2 = points[i], points[i + 1]
        p3 = points[i + 2] if i + 2 < len(points) else p2
        c1 = (p1[0] + (p2[0] - p0[0]) / 6, p1[1] + (p2[1] - p0[1]) / 6)
        c2 = (p2[0] - (p3[0] - p1[0]) / 6, p2[1] - (p3[1] - p1[1]) / 6)
        d.append(f"C{c1[0]:.1f},{c1[1]:.1f} {c2[0]:.1f},{c2[1]:.1f} {p2[0]:.1f},{p2[1]:.1f}")
    if close_to_bottom:
        d.append(f"L{W},{H} L0,{H} Z")
    return " ".join(d)


def ridge_line(rng, base, amp, waves, phase):
    pts = []
    n = 18
    for i in range(n + 1):
        x = W * i / n
        y = base
        for k, (freq, a) in enumerate(waves):
            y += amp * a * math.sin(2 * math.pi * freq * i / n + phase + k)
        y += rng.uniform(-amp * 0.08, amp * 0.08)
        pts.append((x, y))
    return pts


def ridge(name, sky_top, sky_bottom, layers, sun=None, seed=7):
    rng = random.Random(seed)
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
           '<defs><linearGradient id="sky" x1="0" y1="0" x2="0" y2="1">'
           f'<stop offset="0" stop-color="{sky_top}"/><stop offset="1" stop-color="{sky_bottom}"/>'
           '</linearGradient></defs>',
           f'<rect width="{W}" height="{H}" fill="url(#sky)"/>']
    if sun:
        cx, cy, r, color, op = sun
        out.append(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{color}" opacity="{op}"/>')
    for i, color in enumerate(layers):
        base = H * (0.50 + 0.105 * i)
        amp = 70 - i * 8
        waves = [(1.1 + 0.3 * i, 1.0), (2.7 + 0.2 * i, 0.35), (5.3, 0.12)]
        pts = ridge_line(rng, base, amp, waves, phase=1.7 * i + 0.4)
        out.append(f'<path d="{smooth_path(pts)}" fill="{color}"/>')
    out.append("</svg>")
    with open(os.path.join(HERE, name + ".svg"), "w") as f:
        f.write("\n".join(out))


def contour(name, bg, line, seed=3, levels=22):
    """Topographic map: iso-lines of a smooth height field (marching squares)."""
    rng = random.Random(seed)
    hills = [(rng.uniform(0.1, 0.9) * W, rng.uniform(0.1, 0.9) * H,
              rng.uniform(260, 620), rng.uniform(0.5, 1.0)) for _ in range(7)]

    def height(x, y):
        v = 0.0
        for (hx, hy, s, a) in hills:
            dx, dy = (x - hx) / (s * 1.3), (y - hy) / s
            v += a * math.exp(-(dx * dx + dy * dy))
        v += 0.08 * math.sin(x / 310.0) * math.cos(y / 270.0)
        return v

    gx, gy = 320, 180
    sx, sy = W / gx, H / gy
    grid = [[height(i * sx, j * sy) for i in range(gx + 1)] for j in range(gy + 1)]
    lo = min(min(r) for r in grid)
    hi = max(max(r) for r in grid)

    def interp(p1, p2, v1, v2, t):
        f = 0.5 if v1 == v2 else (t - v1) / (v2 - v1)
        return (p1[0] + f * (p2[0] - p1[0]), p1[1] + f * (p2[1] - p1[1]))

    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
           f'<rect width="{W}" height="{H}" fill="{bg}"/>',
           f'<g fill="none" stroke="{line}" stroke-linecap="round">']
    for k in range(1, levels):
        t = lo + (hi - lo) * k / levels
        segs = []
        for j in range(gy):
            for i in range(gx):
                c = [(i * sx, j * sy), ((i + 1) * sx, j * sy), ((i + 1) * sx, (j + 1) * sy), (i * sx, (j + 1) * sy)]
                v = [grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]]
                pts = []
                for a in range(4):
                    b = (a + 1) % 4
                    if (v[a] < t) != (v[b] < t):
                        pts.append(interp(c[a], c[b], v[a], v[b], t))
                if len(pts) == 2:
                    segs.append(pts)
                elif len(pts) == 4:
                    segs.append(pts[:2])
                    segs.append(pts[2:])
        d = " ".join(f"M{a[0]:.1f},{a[1]:.1f}L{b[0]:.1f},{b[1]:.1f}" for a, b in segs)
        major = k % 5 == 0
        op = 0.55 if major else 0.28
        width = 2.6 if major else 1.6
        out.append(f'<path d="{d}" stroke-width="{width}" opacity="{op}"/>')
    out.append("</g></svg>")
    with open(os.path.join(HERE, name + ".svg"), "w") as f:
        f.write("\n".join(out))


def main():
    ridge("ridge", "#DCE9E4", "#F4EEE3",
          ["#A9CBC1", "#7FB0A4", "#4F8E82", "#2D6B61", "#1A4A43"],
          sun=(1880, 470, 120, "#F7E3C0", 0.9), seed=11)
    ridge("ridge-night", "#0E1A1F", "#1C2D33",
          ["#22383D", "#1C3035", "#16282C", "#112124", "#0B1719"],
          sun=(1880, 430, 90, "#E9E2CF", 0.85), seed=11)
    ridge("dune", "#F3E6D3", "#EBD5B5",
          ["#E2C49A", "#D3AB78", "#C2925C", "#A87643", "#8A5D31"],
          sun=(640, 420, 140, "#FFF4E2", 0.8), seed=23)
    contour("contour", "#123C38", "#5BC7B4", seed=5)
    contour("contour-light", "#EEF2EF", "#7FA59C", seed=9)
    print("wallpapers written")


if __name__ == "__main__":
    main()
