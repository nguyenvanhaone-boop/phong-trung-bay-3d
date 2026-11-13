#!/usr/bin/env python3
"""Tao anh texture cho phong trung bay (DHMTPTB3-41). Chi dung thu vien chuan cua Python.

    python3 tools/gen_textures.py      -> assets/textures/san_go.png, assets/textures/da_cam_thach.png

Moi anh 256 x 256 ung voi 1 m x 1 m (hinh hop dat u, v theo met nen anh lap lai moi 1 m).
Anh lap lien mach: canh trai noi canh phai, canh tren noi canh duoi.
"""
import math
import os
import random
import struct
import zlib

SIZE = 256
OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "textures")


def write_png(path, pixels, w, h):
    """Ghi PNG RGB 8 bit (khong nen bang bo loc, chi zlib)."""
    raw = b"".join(b"\x00" + bytes(pixels[y * w * 3:(y + 1) * w * 3]) for y in range(h))

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def tileable_noise(seed, period):
    """Value noise lap theo chu ky `period` o luoi."""
    rnd = random.Random(seed)
    grid = [[rnd.random() for _ in range(period)] for _ in range(period)]

    def smooth(t):
        return t * t * (3 - 2 * t)

    def noise(x, y):  # x, y trong [0, period)
        x0, y0 = int(math.floor(x)) % period, int(math.floor(y)) % period
        x1, y1 = (x0 + 1) % period, (y0 + 1) % period
        tx, ty = smooth(x - math.floor(x)), smooth(y - math.floor(y))
        a = grid[y0][x0] * (1 - tx) + grid[y0][x1] * tx
        b = grid[y1][x0] * (1 - tx) + grid[y1][x1] * tx
        return a * (1 - ty) + b * ty

    return noise


def fbm(noise, x, y, octaves, period):
    total, amp, freq, norm = 0.0, 1.0, 1, 0.0
    for _ in range(octaves):
        total += amp * noise((x * freq) % period, (y * freq) % period)
        norm += amp
        amp *= 0.5
        freq *= 2
    return total / norm


def clamp8(v):
    return max(0, min(255, int(round(v))))


def wood():
    """San go: 5 tam van ngang, moi tam rong 20 cm, mau hoi khac nhau, ranh toi giua cac tam."""
    planks = 5
    rnd = random.Random(7)
    tints = [0.85 + 0.3 * rnd.random() for _ in range(planks)]
    offsets = [rnd.random() * 8 for _ in range(planks)]
    noise = tileable_noise(11, 8)
    px = []
    for y in range(SIZE):
        p = y * planks // SIZE
        local = (y * planks / SIZE) - p  # vi tri trong tam van [0, 1)
        for x in range(SIZE):
            u = x / SIZE * 8
            # Van go: soc doc theo chieu dai tam, uon nhe theo nhieu.
            g = fbm(noise, (u + offsets[p]) % 8, local * 2 + p * 1.7, 3, 8)
            grain = 0.5 + 0.5 * math.sin((local * 18 + g * 6) * math.pi)
            base = 0.75 + 0.25 * grain
            seam = 0.45 if local < 0.025 or local > 0.975 else 1.0
            k = base * tints[p] * seam
            px += [clamp8(178 * k), clamp8(122 * k), clamp8(78 * k)]
    return px


def marble():
    """Da cam thach xam sang voi gan toi."""
    noise = tileable_noise(23, 8)
    px = []
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE * 8, y / SIZE * 8
            t = fbm(noise, u, v, 5, 8)
            vein = abs(math.sin((u / 8 + v / 8 + 2.5 * t) * 2 * math.pi))
            vein = vein ** 0.25  # gan manh, nen rong
            k = 0.62 + 0.38 * vein
            px += [clamp8(222 * k), clamp8(222 * k), clamp8(228 * k)]
    return px


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name, gen in (("san_go.png", wood), ("da_cam_thach.png", marble)):
        path = os.path.normpath(os.path.join(OUT, name))
        write_png(path, gen(), SIZE, SIZE)
        print("da tao", path)
