#!/usr/bin/env python3
"""Riddim Lab artwork generator.
Every texture, knob, meter face, reel and thumbnail in Resources/ is produced by this script
(numpy + Pillow), so the art can be re-rendered or tweaked. Usage: python3 gen_assets.py <out_dir> <fonts_dir>
fonts_dir must contain BarlowCondensed-*.ttf and BebasNeue-Regular.ttf"""
import sys, os, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

OUT = sys.argv[1] if len(sys.argv) > 1 else "Resources"
FONTS = sys.argv[2] if len(sys.argv) > 2 else "fonts"
os.makedirs(OUT, exist_ok=True)

def save(arr_or_img, name):
    im = arr_or_img if isinstance(arr_or_img, Image.Image) else Image.fromarray(np.clip(arr_or_img, 0, 255).astype(np.uint8))
    im.save(os.path.join(OUT, name), optimize=True)

def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), int(size))

# ---------------------------------------------------------------- noise helpers
def tnoise(h, w, sx, sy, seed):
    """tileable gaussian noise, low-passed with feature size (sx, sy) pixels"""
    r = np.random.default_rng(seed)
    F = np.fft.fft2(r.standard_normal((h, w)))
    fy = np.fft.fftfreq(h)[:, None]; fx = np.fft.fftfreq(w)[None, :]
    F *= np.exp(-((fx * sx) ** 2 + (fy * sy) ** 2))
    n = np.real(np.fft.ifft2(F)); n = (n - n.mean()) / (n.std() + 1e-9)
    return n

def fbm(h, w, seed, parts):
    n = np.zeros((h, w))
    for i, (sx, sy, wt) in enumerate(parts):
        n += wt * tnoise(h, w, sx, sy, seed * 31 + i)
    return n

def tile_scratches(h, w, count, seed, strength, length=(20, 90)):
    im = Image.new("L", (w, h), 0); d = ImageDraw.Draw(im); r = np.random.default_rng(seed)
    for _ in range(count):
        x, y = r.uniform(0, w), r.uniform(0, h); a = r.uniform(0, math.pi); L = r.uniform(*length)
        dx, dy = math.cos(a) * L, math.sin(a) * L; v = int(r.uniform(0.3, 1.0) * 255)
        for ox in (-w, 0, w):
            for oy in (-h, 0, h):
                d.line([(x + ox, y + oy), (x + dx + ox, y + dy + oy)], fill=v, width=1)
    return np.asarray(im.filter(ImageFilter.GaussianBlur(0.5)), float) / 255.0 * strength

def speckle(h, w, density, seed, blur=0.7):
    r = np.random.default_rng(seed)
    m = (r.random((h, w)) < density).astype(float)
    m = np.asarray(Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(blur)), float) / 255.0
    return m / (m.max() + 1e-9)

def blend(base, color, a):
    return base * (1 - a[..., None]) + np.array(color, float) * a[..., None]

# ---------------------------------------------------------------- textures (512 tiles)
def tex_green(seed=1, base=(30, 62, 43)):
    h = w = 512
    n = fbm(h, w, seed, [(70, 70, 1.0), (14, 14, 0.7), (2.2, 2.2, 0.45)])
    col = np.array(base, float)[None, None, :] * (1 + 0.075 * n[..., None] / 1.5)
    # faint vertical brushed paint streaks
    st = tnoise(h, w, 1.2, 40, seed + 9)
    col *= (1 + 0.018 * st[..., None])
    # worn patches show dull lighter green-grey metal
    wear = np.clip((fbm(h, w, seed + 3, [(30, 30, 1)]) - 1.15) * 0.8, 0, 0.5)
    col = blend(col, (88, 108, 86), wear * 0.28)
    # brown stains
    stain = np.clip((fbm(h, w, seed + 5, [(45, 90, 1)]) - 1.0) * 0.4, 0, 0.22)
    col = blend(col, (46, 38, 22), stain)
    col = blend(col, (150, 160, 140), tile_scratches(h, w, 90, seed, 0.18))
    col = blend(col, (14, 20, 14), tile_scratches(h, w, 60, seed + 1, 0.28, (10, 50)))
    col = blend(col, (120, 140, 118), speckle(h, w, 0.0016, seed + 2) * 0.55)
    col = blend(col, (10, 16, 10), speckle(h, w, 0.003, seed + 4) * 0.4)
    return col

def tex_brown(seed=2):
    h = w = 512
    base = np.array((74, 42, 26), float)
    n = fbm(h, w, seed, [(60, 40, 1.0), (10, 6, 0.6), (2, 2, 0.35)])
    col = base[None, None, :] * (1 + 0.10 * n[..., None] / 1.5)
    st = tnoise(h, w, 60, 1.0, seed + 7)
    col *= (1 + 0.05 * st[..., None])
    rust = np.clip((fbm(h, w, seed + 3, [(35, 35, 1), (8, 8, 0.5)]) - 0.6) * 0.5, 0, 0.6)
    col = blend(col, (150, 72, 28), rust * 0.32)
    col = blend(col, (30, 18, 12), np.clip((fbm(h, w, seed + 4, [(20, 20, 1)]) - 1.3) * 0.6, 0, 0.3))
    col = blend(col, (190, 150, 100), tile_scratches(h, w, 80, seed, 0.16))
    col = blend(col, (200, 120, 50), speckle(h, w, 0.0018, seed + 2) * 0.6)
    return col

def tex_dark(seed=3):
    h = w = 512
    n = fbm(h, w, seed, [(40, 40, 1.0), (4, 4, 0.5), (1.2, 1.2, 0.4)])
    col = np.array((24, 25, 23), float)[None, None, :] * (1 + 0.14 * n[..., None] / 1.5)
    st = tnoise(h, w, 50, 0.9, seed + 7); col *= (1 + 0.06 * st[..., None])
    col = blend(col, (120, 116, 100), tile_scratches(h, w, 70, seed, 0.10))
    return col

def tex_wood(seed=4):
    h = w = 512
    g = fbm(h, w, seed, [(180, 3.0, 1.0), (60, 1.2, 0.6), (10, 0.6, 0.25)])
    ring = np.sin((np.arange(h)[:, None] * 0.18 + 6 * tnoise(h, w, 150, 14, seed + 1)) )
    t = np.clip(0.5 + 0.22 * g + 0.10 * ring, 0, 1)[..., None]
    dark = np.array((34, 18, 10), float); light = np.array((116, 70, 36), float)
    col = dark + (light - dark) * t
    pores = speckle(h, w, 0.01, seed + 2, 0.6)
    col = blend(col, (14, 8, 4), pores * 0.5)
    return col

def tex_brass(seed=5):
    h = w = 512
    g = fbm(h, w, seed, [(90, 0.7, 1.0), (30, 0.5, 0.6), (8, 0.4, 0.35)])
    t = np.clip(0.52 + 0.20 * g, 0, 1)[..., None]
    dark = np.array((110, 78, 30), float); light = np.array((226, 186, 104), float)
    col = dark + (light - dark) * t
    col = blend(col, (255, 235, 170), tile_scratches(h, w, 70, seed, 0.22, (30, 150)))
    col = blend(col, (70, 48, 18), speckle(h, w, 0.002, seed + 3) * 0.5)
    return col


def make_textures():
    save(tex_green(1), "tex_green.png")
    save(tex_green(11, (36, 66, 40)), "tex_green2.png")
    save(tex_brown(2), "tex_brown.png")
    save(tex_dark(3), "tex_dark.png")
    save(tex_wood(4), "tex_wood.png")
    save(tex_brass(5), "tex_brass.png")

# ---------------------------------------------------------------- colour ramps
def ramp(t, stops):
    """t: array 0..1 ; stops: list of (pos,(r,g,b))"""
    t = np.clip(t, 0, 1)
    out = np.zeros(t.shape + (3,))
    pos = [p for p, _ in stops]
    for c in range(3):
        out[..., c] = np.interp(t, pos, [col[c] for _, col in stops])
    return out

BRASS = [(0.0, (46, 30, 10)), (0.3, (120, 84, 32)), (0.55, (190, 146, 70)), (0.8, (240, 208, 134)), (1.0, (255, 245, 205))]

def ss_down(arr, size):
    """arr float (H,W,4) premultiplied-alpha-safe downscale with Pillow"""
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGBA")
    return im.resize(size, Image.LANCZOS)

def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0, 1); return t * t * (3 - 2 * t)

# ---------------------------------------------------------------- knob
def render_knob(S=1024):
    """top-down view of a black knurled knob with a brass cap, lit from the upper left.
    The knob radius is 0.38*S, so the sprite is 1/0.76 = 1.316 knob diameters wide."""
    Ro = 0.38 * S
    y, x = np.mgrid[0:S, 0:S].astype(float)
    cx = cy = S / 2
    u = (x - cx) / Ro; v = (y - cy) / Ro
    r = np.hypot(u, v); th = np.arctan2(v, u)
    L = np.array([-0.62, -0.78])
    rgb = np.zeros((S, S, 3)); alpha = np.zeros((S, S))

    # ---- drop shadow (blurred disc, offset down/right)
    sh = (np.hypot(u - 0.05, v - 0.13) < 1.0).astype(float)
    sh = np.asarray(Image.fromarray((sh * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(S * 0.028)), float) / 255
    alpha = sh * 0.62

    # ---- skirt: knurled rubber
    N = 44
    tilt = 0.55 * np.sin(N * th)
    nx, ny = np.cos(th + tilt * 0.35), np.sin(th + tilt * 0.35)
    ndl = nx * L[0] + ny * L[1]
    skirt_mask = smoothstep(1.0, 0.985, r)
    edge_fall = smoothstep(1.0, 0.80, r)                       # outer edge rolls off darker
    diff = np.clip(ndl, 0, 1)
    ridge_spec = np.power(np.clip(ndl, 0, 1), 14) * (0.55 + 0.45 * np.sin(N * th)) * 0.55
    sb = 0.10 + 0.20 * diff * (0.5 + 0.5 * edge_fall) + ridge_spec * (r > 0.78)
    sb *= (0.70 + 0.30 * edge_fall)
    top = 0.24 * np.clip(1 - np.abs((r - 0.77) / 0.05), 0, 1)  # lit bevel where skirt meets cap
    skirt = np.stack([sb * 255 * 0.95, sb * 255 * 0.95, sb * 255 * 0.92], -1) + top[..., None] * 120
    inside = r <= 1.0
    rgb = np.where(inside[..., None], skirt, rgb)
    alpha = np.where(inside, np.maximum(alpha, skirt_mask), alpha)
    # rim highlight (thin, upper-left)
    rimh = np.exp(-((r - 0.975) / 0.012) ** 2) * np.clip(-(np.cos(th) * L[0] * -1 + np.sin(th) * L[1] * -1) * -1, 0, 1)
    rimh = np.clip(rimh, 0, 1) * np.clip(ndl, 0, 1)
    rgb += rimh[..., None] * 140

    # ---- dark gap between skirt and cap
    gap = np.exp(-((r - 0.735) / 0.016) ** 2)
    rgb *= (1 - 0.85 * gap[..., None])

    # ---- brass cap
    cap = r <= 0.715
    rr = np.minimum(r / 0.715, 1.0)
    th0 = np.arctan2(L[1], L[0])
    lobe = 0.5 + 0.5 * np.cos(2 * (th - th0))
    lobe = np.power(lobe, 1.6)
    rng = np.random.default_rng(3)
    ring_noise = rng.standard_normal(int(S * 0.5))
    ring_noise = np.convolve(ring_noise, np.ones(2) / 2, mode="same")
    idx = np.clip((r * S * 0.5 / 0.715 * 0.9).astype(int), 0, len(ring_noise) - 1)
    brush = ring_noise[idx] * 0.05
    dome = 0.22 * (1 - rr ** 2) * (0.55 - 0.45 * (u * L[0] * -1 + v * L[1] * -1) / np.maximum(r, 1e-3) * -1)
    b = 0.22 + 0.62 * lobe * (0.55 + 0.45 * rr) + brush + dome
    # darker towards the edge (bevel) with lit upper-left lip
    lip = np.exp(-((rr - 0.96) / 0.045) ** 2)
    b = b - 0.22 * lip * (1 - np.clip(ndl, 0, 1)) + 0.30 * lip * np.clip(ndl, 0, 1)
    capcol = ramp(b, BRASS)
    # centre dimple
    dimple = np.exp(-(r / 0.11) ** 2)
    capcol *= (1 - 0.30 * dimple[..., None])
    hl = np.exp(-(((u + 0.17) ** 2 + (v + 0.20) ** 2) / 0.012)) * 0.35
    capcol += hl[..., None] * 90
    rgb = np.where(cap[..., None], capcol, rgb)
    return np.dstack([np.clip(rgb, 0, 255), np.clip(alpha, 0, 1) * 255])

def make_knobs():
    big = render_knob(1024)
    for name, d in (("xl", 80), ("l", 68), ("m", 58), ("s", 46)):
        px = int(round(d * 1.316 * 2))
        save(ss_down(big, (px, px)), f"knob_{name}.png")

# ---------------------------------------------------------------- screws
def render_screw(S=256, kind="brass", angle=0.0):
    y, x = np.mgrid[0:S, 0:S].astype(float)
    u = (x - S / 2) / (S * 0.42); v = (y - S / 2) / (S * 0.42)
    r = np.hypot(u, v)
    L = np.array([-0.62, -0.78])
    nz = np.sqrt(np.clip(1 - r ** 2 * 0.8, 0, 1))
    ndl = (-(u * L[0] + v * L[1]) * -1) * 0.4 + nz * 0.8
    b = np.clip(0.25 + 0.55 * ndl, 0, 1.1)
    cols = BRASS if kind == "brass" else [(0.0, (20, 20, 20)), (0.5, (110, 112, 108)), (1.0, (235, 238, 232))]
    rgb = ramp(b, cols)
    # slot / cross
    ca, sa = math.cos(angle), math.sin(angle)
    ur = u * ca + v * sa; vr = -u * sa + v * ca
    slot = ((np.abs(ur) < 0.11) | (np.abs(vr) < 0.11)) & (r < 0.78)
    shade = np.where(slot, 0.18, 1.0)
    rgb = rgb * shade[..., None]
    # slot lower-right lip highlight
    lip = slot & (((ur + vr) > 0.0)) & (np.minimum(np.abs(ur), np.abs(vr)) > 0.07)
    rgb += lip[..., None] * 20
    # outer ring shadow
    ring = np.exp(-((r - 0.97) / 0.06) ** 2)
    rgb *= (1 - 0.6 * ring[..., None])
    alpha = smoothstep(1.0, 0.94, r) * 255
    return np.dstack([rgb, alpha])

def make_screws():
    for i, ang in enumerate((0.35, 1.0, 2.1, 2.7)):
        save(ss_down(render_screw(256, "brass", ang), (48, 48)), f"screw_brass{i}.png")
        save(ss_down(render_screw(256, "steel", ang), (48, 48)), f"screw_steel{i}.png")


# ---------------------------------------------------------------- VU meters
VU_A0, VU_A1 = -42.0, 42.0          # needle sweep in degrees from vertical (must match the editor)

def vu_pos(db):   return (10 ** (db / 20)) / (10 ** (3 / 20))          # VU: linear in voltage, +3 VU = full scale
def gr_pos(g):    return min(max(g / 20.0, 0), 1) ** 0.6                  # gain reduction scale

def render_meter(w, h, kind, scale=4):
    """returns (face RGB image, overlay RGBA image) at logical size (w,h) * 2 pixels"""
    W, H = w * scale, h * scale
    y, x = np.mgrid[0:H, 0:W].astype(float)
    # bezel: dark gunmetal with a brass hairline
    face = np.zeros((H, W, 3))
    m = 6 * scale
    # warm backlit paper: brighter at lower centre
    gx = (x - W / 2) / (W * 0.62); gy = (y - H * 1.0) / (H * 1.05)
    d = np.hypot(gx, gy)
    t = np.clip(1 - d, 0, 1) ** 0.8
    face = ramp(t, [(0.0, (190, 128, 52)), (0.35, (232, 178, 88)), (0.75, (252, 222, 142)), (1.0, (255, 240, 178))])
    paper = tnoise(H, W, 0.9 * scale, 0.9 * scale, 21)
    face *= (1 + 0.012 * paper[..., None])
    img = Image.fromarray(np.clip(face, 0, 255).astype(np.uint8), "RGB")
    dr = ImageDraw.Draw(img, "RGBA")
    px, py = W / 2, H * 1.06
    R = H * 0.86
    def pt(angle_deg, rad):
        a = math.radians(angle_deg); return (px + math.sin(a) * rad, py - math.cos(a) * rad)
    ink = (62, 32, 12, 255); red = (186, 34, 26, 255)
    if kind == "vu":
        majors = [(-20, "-20"), (-10, "-10"), (-7, "-7"), (-5, "-5"), (-3, "-3"), (-2, "-2"), (-1, "-1"), (0, "0"), (1, "+1"), (2, "+2"), (3, "+3")]
        minors = [-15, -9, -8, -6, -4]
        posf = lambda v: VU_A0 + vu_pos(v) * (VU_A1 - VU_A0)
        red_from, red_to = 0, 3
    else:
        majors = [(0, "0"), (2, "2"), (4, "4"), (6, "6"), (8, "8"), (10, "10"), (15, "15"), (20, "20")]
        minors = [1, 3, 5, 7, 9, 12.5, 17.5]
        posf = lambda v: VU_A1 - gr_pos(v) * (VU_A1 - VU_A0)
        red_from, red_to = 10, 20
    # scale arc
    steps = 80
    arc = [pt(posf(red_from if False else 0) if False else VU_A0 + (VU_A1 - VU_A0) * k / steps, R) for k in range(steps + 1)]
    dr.line(arc, fill=ink, width=int(1.6 * scale))
    # red zone band
    a_red0 = posf(red_from); a_red1 = posf(red_to)
    lo, hi = sorted((a_red0, a_red1))
    band_out = [pt(lo + (hi - lo) * k / 20, R + 0.0 * scale) for k in range(21)]
    band_in = [pt(lo + (hi - lo) * k / 20, R - 5.0 * scale) for k in range(21)][::-1]
    dr.polygon(band_out + band_in, fill=red)
    fnt = font("BarlowCondensed-SemiBold.ttf", 9.6 * scale)
    for v, lab in majors:
        a = posf(v)
        isred = (red_from <= v <= red_to)
        dr.line([pt(a, R - 5.5 * scale if isred else R - 5.0 * scale), pt(a, R + 4.5 * scale)], fill=red if isred else ink, width=int(1.5 * scale))
        tx, ty = pt(a, R + 10.5 * scale)
        dr.text((tx, ty), lab, font=fnt, fill=red if isred else ink, anchor="mm")
    for v in minors:
        a = posf(v); dr.line([pt(a, R - 1.0 * scale), pt(a, R + 3.0 * scale)], fill=ink, width=int(1.0 * scale))
    big = font("BarlowCondensed-SemiBold.ttf", 25 * scale)
    dr.text((W / 2, H * 0.60), "VU" if kind == "vu" else "GR", font=big, fill=(70, 36, 14, 235), anchor="mm")
    small = font("BarlowCondensed-Medium.ttf", 7.6 * scale)
    dr.text((W / 2, H * 0.80), "VOLUME UNITS" if kind == "vu" else "GAIN REDUCTION dB", font=small, fill=(90, 50, 20, 200), anchor="mm")
    # recessed top shadow + side vignette
    a = np.asarray(img, float)
    top = np.clip(1 - y / (H * 0.30), 0, 1) ** 1.6
    side = np.clip(np.abs(x - W / 2) / (W / 2) - 0.78, 0, 1) * 4
    a *= (1 - 0.55 * top[..., None]) * (1 - 0.35 * np.clip(side, 0, 1)[..., None])
    face_rgb = a
    # bezel frame
    frame = np.zeros((H, W, 3)); fy, fx = y, x
    edge = np.minimum(np.minimum(fx, W - 1 - fx), np.minimum(fy, H - 1 - fy))
    inner = edge >= m
    fcol = np.zeros((H, W, 3)) + np.array((26, 26, 24), float)
    fcol += 18 * (1 - np.clip(y / H, 0, 1))[..., None]
    hair = np.exp(-((edge - m * 0.55) / (0.7 * scale)) ** 2)
    fcol = fcol * (1 - 0.0) + hair[..., None] * np.array((150, 112, 52))
    out = np.where(inner[..., None], face_rgb, fcol)
    # inner bevel shadow just inside the frame
    bev = np.exp(-((edge - m) / (2.0 * scale)) ** 2)
    out = np.where(inner[..., None], out * (1 - 0.5 * (bev * (edge >= m))[..., None]), out)
    face_img = Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGB").resize((w * 2, h * 2), Image.LANCZOS)

    # overlay: glass reflection + pivot cover
    ov = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    od = ImageDraw.Draw(ov, "RGBA")
    # diagonal gloss on upper left half of the glass
    gloss = Image.new("L", (W, H), 0); gd = ImageDraw.Draw(gloss)
    gd.polygon([(m, m), (W * 0.62, m), (W * 0.30, H - m), (m, H - m)], fill=40)
    gloss = gloss.filter(ImageFilter.GaussianBlur(3 * scale))
    ov.paste((255, 255, 255, 255), (0, 0), gloss)
    od.line([(m + 2 * scale, m + 1 * scale), (W - m - 2 * scale, m + 1 * scale)], fill=(255, 255, 255, 38), width=int(1.2 * scale))
    # pivot cover
    pr = H * 0.115
    cover = Image.new("L", (W, H), 0); cd = ImageDraw.Draw(cover)
    cd.ellipse([px - pr, py - pr, px + pr, py + pr], fill=255)
    cov = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    cy_ = np.clip((y - (py - pr)) / (2 * pr), 0, 1)
    cc = ramp(1 - cy_, [(0, (12, 12, 12)), (0.6, (48, 46, 42)), (1, (96, 92, 80))])
    covarr = np.dstack([cc, np.zeros((H, W))])
    cov = Image.fromarray(np.clip(np.dstack([cc, np.asarray(cover, float)]), 0, 255).astype(np.uint8), "RGBA")
    ov.alpha_composite(cov)
    # clip pivot cover to the face (anything below the frame is cut)
    clipm = Image.new("L", (W, H), 0); ImageDraw.Draw(clipm).rectangle([m, m, W - m, H - m], fill=255)
    a = np.asarray(ov, float).copy(); a[..., 3] *= np.asarray(clipm, float) / 255.0
    ov = Image.fromarray(a.astype(np.uint8), "RGBA").resize((w * 2, h * 2), Image.LANCZOS)
    return face_img, ov

def make_meters():
    for tag, (w, h) in (("big", (200, 122)), ("head", (160, 100))):
        for kind in ("vu", "gr"):
            f, o = render_meter(w, h, kind)
            save(f, f"meter_{kind}_{tag}_face.png"); save(o, f"meter_{tag}_glass.png") if kind == "vu" else None

# ---------------------------------------------------------------- reels + tape deck
def render_reel(S=512):
    y, x = np.mgrid[0:S, 0:S].astype(float)
    u = (x - S / 2) / (S * 0.48); v = (y - S / 2) / (S * 0.48)
    r = np.hypot(u, v); th = np.arctan2(v, u)
    L = np.array([-0.62, -0.78])
    # flange
    ndl = np.clip(-(u * L[0] + v * L[1]) * -1 / np.maximum(r, 1e-3), -1, 1)
    b = 0.50 + 0.18 * ndl * smoothstep(0.55, 1.0, r) + 0.06 * np.sin(r * 90)
    groove = np.exp(-((r - 0.90) / 0.012) ** 2) * 0.35 + np.exp(-((r - 0.84) / 0.01) ** 2) * 0.25
    b -= groove
    b += 0.20 * np.exp(-((r - 0.985) / 0.02) ** 2) * np.clip(ndl, 0, 1)
    rgb = ramp(b, BRASS)
    alpha = np.where(r <= 1.0, 255.0, 0.0)
    # three windows (transparent) between spokes
    win = np.zeros_like(r, bool)
    for k in range(3):
        c = math.radians(90 + 120 * k)
        dth = np.angle(np.exp(1j * (th - c)))
        # window: sector of +-33 deg, radially 0.30..0.80, softened corners
        inw = (np.abs(dth) < math.radians(34) * (0.55 + 0.45 * smoothstep(0.30, 0.55, r))) & (r > 0.30) & (r < 0.80)
        win |= inw
    # inner edge shadow around windows
    winf = np.asarray(Image.fromarray((win * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(S * 0.004)), float) / 255
    alpha = alpha * (1 - np.clip(winf * 1.6, 0, 1))
    edge = np.asarray(Image.fromarray((win * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(S * 0.012)), float) / 255
    rgb *= (1 - 0.45 * np.clip(edge - winf * 0.0, 0, 1)[..., None] * (~win)[..., None])
    # hub
    hub = r < 0.26
    hb = 0.40 + 0.35 * np.clip(ndl, 0, 1) * 0.6 + 0.05 * np.sin(r * 160)
    hubcol = ramp(hb, [(0, (14, 14, 14)), (0.5, (96, 94, 88)), (1, (210, 206, 192))])
    rgb = np.where(hub[..., None], hubcol, rgb)
    ring = np.exp(-((r - 0.26) / 0.012) ** 2)
    rgb *= (1 - 0.7 * ring[..., None])
    spindle = r < 0.07
    rgb = np.where(spindle[..., None], np.array((8, 8, 8.0)), rgb)
    for k in range(3):
        c = math.radians(30 + 120 * k)
        hx, hy = math.cos(c) * 0.17, math.sin(c) * 0.17
        hole = np.hypot(u - hx, v - hy) < 0.035
        rgb = np.where(hole[..., None], np.array((12, 12, 12.0)), rgb)
    return np.dstack([rgb, alpha])

def make_reel():
    big = render_reel(512)
    save(ss_down(big, (208, 208)), "reel.png")

def make_deck(w=330, h=96):
    sc = 4
    W, H = w * sc, h * sc
    y, x = np.mgrid[0:H, 0:W].astype(float)
    # recessed dark plate with brass frame
    n = fbm(H, W, 8, [(40, 40, 1), (3, 3, 0.4)])
    base = np.array((26, 24, 22), float)[None, None, :] * (1 + 0.14 * n[..., None] / 1.5)
    base *= (0.8 + 0.3 * (1 - y / H))[..., None]
    img = Image.fromarray(np.clip(base, 0, 255).astype(np.uint8), "RGB")
    dr = ImageDraw.Draw(img, "RGBA")
    def C(px, py): return (px * sc, py * sc)
    def circ(cxl, cyl, rl, **k):
        dr.ellipse([ (cxl - rl) * sc, (cyl - rl) * sc, (cxl + rl) * sc, (cyl + rl) * sc], **k)
    lc, rc = (60, 48), (270, 48)
    # tape path (behind posts)
    path = [(lc[0] + 33, 72), (108, 80), (138, 84), (192, 84), (222, 80), (rc[0] - 25, 68)]
    dr.line([C(*p) for p in path], fill=(52, 30, 20, 255), width=int(2.4 * sc), joint="curve")
    dr.line([C(p[0], p[1] - 0.9) for p in path], fill=(120, 76, 44, 150), width=int(0.7 * sc), joint="curve")
    # tape packs: glossy brown discs (left fuller than right)
    for (cx_, cy_), rad in ((lc, 38), (rc, 30)):
        circ(cx_, cy_, rad, fill=(46, 26, 16, 255))
        for k in range(1, 9):
            circ(cx_, cy_, rad - k * 1.6, outline=(88, 54, 32, 120), width=1)
        circ(cx_, cy_, rad, outline=(14, 8, 5, 255), width=int(1.2 * sc))
        # gloss arc
        dr.arc([(cx_ - rad + 3) * sc, (cy_ - rad + 3) * sc, (cx_ + rad - 3) * sc, (cy_ + rad - 3) * sc], 200, 280, fill=(190, 140, 100, 90), width=int(2 * sc))
    # head block with three heads
    dr.rounded_rectangle([C(146, 76), C(184, 92)], radius=2 * sc, fill=(70, 66, 60, 255), outline=(20, 18, 16, 255), width=sc)
    for k, hx in enumerate((152, 162, 172)):
        dr.rounded_rectangle([C(hx, 78), C(hx + 8, 91)], radius=sc, fill=(150, 146, 138, 255), outline=(30, 28, 26, 255), width=int(0.8 * sc))
        dr.rectangle([C(hx + 2, 80), C(hx + 6, 82)], fill=(200, 160, 80, 255))
    # capstan + pinch roller
    circ(204, 82, 4.6, fill=(170, 166, 156, 255), outline=(30, 28, 26, 255), width=int(0.9 * sc))
    circ(204, 82, 1.6, fill=(40, 40, 38, 255))
    circ(214, 76, 5.2, fill=(30, 30, 30, 255), outline=(70, 70, 66, 255), width=int(0.9 * sc))
    # guide posts
    for gx, gy in ((108, 80), (130, 84), (232, 78)):
        circ(gx, gy, 2.6, fill=(200, 190, 160, 255), outline=(30, 28, 26, 255), width=int(0.8 * sc))
    # bezel frame in brass
    dr.rounded_rectangle([1.5 * sc, 1.5 * sc, W - 1.5 * sc, H - 1.5 * sc], radius=5 * sc, outline=(120, 88, 38, 255), width=int(2.2 * sc))
    dr.rounded_rectangle([3.5 * sc, 3.5 * sc, W - 3.5 * sc, H - 3.5 * sc], radius=4 * sc, outline=(10, 8, 6, 200), width=int(1.4 * sc))
    a = np.asarray(img, float)
    # inner shadow at top
    top = np.clip(1 - y / (H * 0.35), 0, 1) ** 2
    a *= (1 - 0.5 * top[..., None])
    save(Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), "RGB").resize((w * 2, h * 2), Image.LANCZOS), "tape_deck.png")

# ---------------------------------------------------------------- library thumbnails (illustrated scenes)
TW, TH = 112, 80
def vgrad(top, bot, w=TW, h=TH):
    t = np.linspace(0, 1, h)[:, None, None]
    a = np.array(top, float)[None, None, :] * (1 - t) + np.array(bot, float)[None, None, :] * t
    return Image.fromarray(np.repeat(a, w, axis=1).astype(np.uint8), "RGB")

def bokeh(im, seed, colors, n=14, rmin=3, rmax=11, alpha=70):
    r = np.random.default_rng(seed)
    lay = Image.new("RGBA", im.size, (0, 0, 0, 0)); d = ImageDraw.Draw(lay)
    for _ in range(n):
        x, y = r.uniform(0, im.width), r.uniform(0, im.height * 0.75); rad = r.uniform(rmin, rmax)
        c = colors[r.integers(len(colors))]
        d.ellipse([x - rad, y - rad, x + rad, y + rad], fill=c + (int(alpha * r.uniform(0.5, 1)),))
    lay = lay.filter(ImageFilter.GaussianBlur(1.4))
    out = im.convert("RGBA"); out.alpha_composite(lay); return out.convert("RGB")

def vignette(im, strength=0.55):
    w, h = im.size; y, x = np.mgrid[0:h, 0:w]
    d = np.hypot((x - w / 2) / (w / 2), (y - h / 2) / (h / 2)) / 1.2
    a = np.asarray(im, float) * (1 - strength * np.clip(d, 0, 1) ** 2)[..., None]
    return Image.fromarray(a.astype(np.uint8), "RGB")

def grain(im, amt=7, seed=1):
    a = np.asarray(im, float); n = np.random.default_rng(seed).standard_normal(a.shape[:2])[..., None] * amt
    return Image.fromarray(np.clip(a + n, 0, 255).astype(np.uint8), "RGB")

def thumb_kingston():
    im = vgrad((30, 18, 10), (92, 54, 24)); im = bokeh(im, 1, [(255, 190, 90), (255, 140, 50), (255, 220, 140)])
    d = ImageDraw.Draw(im)
    d.polygon([(8, 80), (22, 46), (90, 46), (104, 80)], fill=(22, 14, 10))
    for row in range(3):
        for k in range(10):
            cx = 28 + k * 6.5 - row * 1.5; cy = 53 + row * 7
            d.ellipse([cx - 1.6, cy - 1.6, cx + 1.6, cy + 1.6], fill=(220, 170, 90))
    d.rectangle([24, 50, 88, 52], fill=(60, 40, 22))
    return im

def thumb_foundation():
    im = vgrad((52, 32, 18), (120, 82, 44)); d = ImageDraw.Draw(im)
    d.rectangle([14, 16, 56, 76], fill=(26, 18, 12), outline=(150, 110, 60))
    d.rectangle([60, 28, 100, 76], fill=(30, 20, 14), outline=(150, 110, 60))
    for cx, cy, r in ((35, 34, 10), (35, 60, 7), (80, 52, 14)):
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(60, 44, 30), outline=(190, 150, 90)); d.ellipse([cx - r / 3, cy - r / 3, cx + r / 3, cy + r / 3], fill=(18, 12, 8))
    return bokeh(im, 2, [(255, 200, 120)], 6, 3, 8, 50)

def reel_scene(bg1, bg2, seed, big=False):
    im = vgrad(bg1, bg2); d = ImageDraw.Draw(im)
    spots = ((34, 40, 26), (80, 40, 22)) if not big else ((40, 42, 32), (92, 52, 18))
    for cx, cy, r in spots:
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(46, 28, 18), outline=(160, 120, 66))
        d.ellipse([cx - r * 0.88, cy - r * 0.88, cx + r * 0.88, cy + r * 0.88], outline=(200, 160, 90))
        for k in range(3):
            a = math.radians(90 + 120 * k)
            d.polygon([(cx, cy), (cx + math.cos(a - 0.5) * r * 0.78, cy + math.sin(a - 0.5) * r * 0.78), (cx + math.cos(a + 0.5) * r * 0.78, cy + math.sin(a + 0.5) * r * 0.78)], fill=(18, 14, 10))
        d.ellipse([cx - 4, cy - 4, cx + 4, cy + 4], fill=(190, 150, 80))
    d.line([(34, 66), (56, 74), (80, 62)], fill=(60, 36, 24), width=3)
    return bokeh(im, seed, [(255, 190, 100)], 6, 3, 8, 45)

def thumb_lovers():
    im = vgrad((250, 150, 120), (120, 40, 70)); d = ImageDraw.Draw(im)
    d.ellipse([36, 26, 76, 66], fill=(255, 214, 140))
    d.rectangle([0, 56, TW, TH], fill=(60, 26, 54))
    for k in range(8):
        d.line([(0, 58 + k * 3), (TW, 58 + k * 3)], fill=(110, 50, 70), width=1)
    for px_, hgt in ((18, 44), (92, 36)):
        d.line([(px_, TH), (px_ + 3, TH - hgt)], fill=(14, 8, 12), width=3)
        for a in (-2.6, -2.0, -1.2, -0.5, 0.2, 0.7):
            d.line([(px_ + 3, TH - hgt), (px_ + 3 + math.cos(a) * 16, TH - hgt + math.sin(a) * 11 + 4)], fill=(14, 8, 12), width=2)
    return im

def thumb_rocksteady():
    im = vgrad((24, 20, 18), (70, 46, 28)); d = ImageDraw.Draw(im)
    cx, cy = 52, 42
    for r_ in range(34, 8, -2): d.ellipse([cx - r_, cy - r_, cx + r_, cy + r_], fill=(14, 14, 14) if r_ % 4 == 0 else (24, 24, 24))
    d.ellipse([cx - 11, cy - 11, cx + 11, cy + 11], fill=(180, 40, 30)); d.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=(10, 10, 10))
    d.line([(100, 10), (82, 30), (64, 36)], fill=(190, 160, 100), width=3)
    return bokeh(im, 5, [(255, 190, 100)], 5, 3, 7, 40)

def thumb_dancehall():
    im = vgrad((255, 110, 40), (90, 10, 20)); d = ImageDraw.Draw(im)
    d.ellipse([46, 30, 80, 64], fill=(255, 190, 80))
    for x0, hgt in ((6, 40), (20, 52), (84, 52), (98, 40)):
        d.rectangle([x0, TH - hgt, x0 + 13, TH], fill=(16, 8, 8))
        for k in range(hgt // 14):
            d.ellipse([x0 + 3, TH - hgt + 3 + k * 14, x0 + 10, TH - hgt + 10 + k * 14], outline=(110, 60, 40))
    return im

def thumb_vocal():
    im = vgrad((14, 14, 16), (50, 36, 26)); d = ImageDraw.Draw(im, "RGBA")
    d.polygon([(56, 0), (20, TH), (92, TH)], fill=(255, 220, 160, 40))
    d.rounded_rectangle([48, 14, 64, 40], radius=8, fill=(40, 36, 32), outline=(190, 150, 90))
    for k in range(5): d.line([(48, 18 + k * 4), (64, 18 + k * 4)], fill=(120, 100, 70))
    d.line([(56, 40), (56, 62)], fill=(80, 72, 62), width=3); d.line([(40, 66), (72, 66)], fill=(80, 72, 62), width=3)
    return im.convert("RGB")

def thumb_mix():
    im = vgrad((70, 16, 14), (20, 8, 8)); d = ImageDraw.Draw(im)
    for k in range(9):
        x0 = 12 + k * 10
        d.rounded_rectangle([x0, 14, x0 + 4, 66], radius=2, fill=(12, 8, 8))
        yy = 20 + ((k * 37) % 36)
        d.rectangle([x0 - 2, yy, x0 + 6, yy + 6], fill=(200, 160, 90), outline=(255, 220, 140))
    return bokeh(im, 9, [(255, 90, 60), (255, 170, 80)], 6, 3, 8, 50)

def make_thumbs():
    fns = [thumb_kingston, thumb_foundation, lambda: reel_scene((40, 38, 22), (110, 86, 40), 3),
           lambda: reel_scene((24, 26, 24), (74, 56, 40), 4, True), thumb_lovers, thumb_rocksteady, thumb_dancehall, thumb_vocal, thumb_mix]
    for i, f in enumerate(fns):
        im = grain(vignette(f()), 5, i)
        save(im, f"thumb{i + 1}.png")

if __name__ == "__main__":
    make_textures(); make_knobs(); make_screws(); make_meters(); make_reel(); make_deck(); make_thumbs()
