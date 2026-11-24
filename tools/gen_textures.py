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


def mix(a, b, t):
    return [a[i] * (1 - t) + b[i] * t for i in range(3)]


DS_W, DS_H = 512, 320


def dong_son():
    """Hoa van dong Dong Son cho trong dong / thap dong (bao tang mo rong).

    Anh quan quanh vat: cot (u) = goc quay, hang = vi tri doc profile. Trong dong co 5 doan profile,
    mat trong la doan cuoi nen chiem 20 % TREN cua anh, hang 0 = tam mat trong. Vi vay hoa van mat
    trong ve theo "ban kinh" r = hang / 64 va "goc" = u: ngoi sao 14 canh o tam roi cac vanh hoa van.
    """
    noise = tileable_noise(31, 8)
    base, high, low = (0.46, 0.38, 0.22), (0.78, 0.64, 0.36), (0.20, 0.17, 0.10)
    patina = (0.26, 0.42, 0.33)
    top = DS_H // 5
    px = []
    for y in range(DS_H):
        for x in range(DS_W):
            u = x / DS_W
            relief = 0.0  # 1 = noi (sang), -1 = chim (toi), 0 = nen
            if y < top:
                r = y / top
                if r < 0.2:  # ngoi sao 14 canh: canh hep dan khi ra xa tam
                    a = (u * 14) % 1.0
                    relief = 1.0 if abs(a - 0.5) < 0.5 * (1 - r / 0.2) ** 0.8 else -0.6
                elif r < 0.24 or 0.40 < r < 0.43 or 0.64 < r < 0.67 or r > 0.95:
                    relief = 1.0
                elif r < 0.40:  # vong tron co cham
                    cx = (u * 40) % 1.0 - 0.5
                    cy = (r - 0.32) / 0.08 * 0.5
                    d = math.hypot(cx, cy)
                    relief = 1.0 if 0.28 < d < 0.4 or d < 0.1 else -0.3
                elif r < 0.64:  # chim bay (net xien lap lai)
                    a = (u * 18 + (r - 0.43) * 3) % 1.0
                    relief = 1.0 if a < 0.12 or abs(a - 0.5) < 0.05 else -0.2
                else:  # rang cua
                    a = (u * 64) % 1.0
                    t = (r - 0.67) / 0.28
                    relief = 1.0 if abs(a - 0.5) < 0.5 * (1 - t) else -0.4
            else:
                t = (y - top) / (DS_H - top) * 6 % 1.0  # 6 dai hoa van tren than
                band = int((y - top) / (DS_H - top) * 6)
                if t < 0.08 or t > 0.92:
                    relief = 1.0
                elif band % 3 == 0:  # thang (vach doc)
                    relief = 1.0 if (u * 90) % 1.0 < 0.3 else -0.3
                elif band % 3 == 1:  # zigzag
                    a = (u * 30) % 1.0
                    zz = abs(a - 0.5) * 2
                    relief = 1.0 if abs((t - 0.15) / 0.7 - zz) < 0.12 else -0.2
                else:  # vong tron dong tam
                    cx = (u * 36) % 1.0 - 0.5
                    d = math.hypot(cx, (t - 0.5) * 0.9)
                    relief = 1.0 if 0.25 < d < 0.35 or d < 0.08 else -0.2
            c = mix(base, high, relief) if relief > 0 else mix(base, low, -relief)
            n = fbm(noise, u * 8, y / DS_H * 8, 4, 8)
            c = mix(c, patina, max(0.0, n - 0.45) * 1.6)
            px += [clamp8(255 * c[0]), clamp8(255 * c[1]), clamp8(255 * c[2])]
    return px


HL = 512


def hoa_lam():
    """Gom hoa lam (men trang, ve xanh coban). Binh co 8 doan profile, moi doan nhan tron
    1 bo hoa van (anh lap 8 lan theo chieu doc): vach, canh sen, day leo, vach doi."""
    white, blue = (0.94, 0.95, 0.97), (0.10, 0.20, 0.55)
    noise = tileable_noise(41, 8)
    px = []
    for y in range(HL):
        t = (y * 8 / HL) % 1.0
        for x in range(HL):
            u = x / HL
            ink = 0.0
            if t < 0.06 or 0.88 < t < 0.91 or t > 0.95:
                ink = 1.0
            elif 0.10 < t < 0.32:  # canh sen
                a = (u * 12) % 1.0
                h = (t - 0.10) / 0.22
                w = 0.45 * (1 - h) ** 0.6
                edge = abs(abs(a - 0.5) - w)
                ink = 1.0 if edge < 0.04 else (0.35 if abs(a - 0.5) < w else 0.0)
            elif 0.36 < t < 0.84:  # day leo: duong sin + la tron o dinh song
                c = 0.60 + 0.16 * math.sin(2 * math.pi * u * 6)
                if abs(t - c) < 0.025:
                    ink = 1.0
                a = (u * 6) % 1.0
                for k, (lx, ly) in enumerate(((0.25, 0.42), (0.75, 0.78))):
                    d = math.hypot((a - lx) * 1.6, t - ly)
                    if d < 0.07:
                        ink = max(ink, 0.85 if d < 0.05 else 1.0)
            n = fbm(noise, u * 8, y / HL * 8, 3, 8)
            c = mix(white, blue, ink * (0.85 + 0.3 * n))
            px += [clamp8(255 * c[0]), clamp8(255 * c[1]), clamp8(255 * c[2])]
    return px


HV = 256


def hoa_van_tuong():
    """Giay dan tuong: luoi tram (diamond) voi bong hoa 8 canh cach dieu o giua moi o, cham tron
    o giao diem. Anh XAM NHAT (nen 0,96, hoa van 0,72-0,82): ma nguon nhan voi kd cua tung khu
    nen 1 anh dung cho tuong kem, do tham, xanh dam. Tuong dat v = y / 2 -> anh lap moi 2 m."""
    px = []
    cells = 4  # 4 x 4 o tram tren 1 anh
    for y in range(HV):
        for x in range(HV):
            u, v = x / HV * cells, y / HV * cells
            fu, fv = u % 1.0 - 0.5, v % 1.0 - 0.5           # toa do trong o, tam o (0, 0)
            g = 0.96
            diamond = abs(abs(fu) + abs(fv) - 0.5)              # canh hinh thoi noi trung diem canh o
            if diamond < 0.018:
                g = 0.80
            r = math.hypot(fu, fv)
            a = math.atan2(fv, fu)
            petal = 0.17 + 0.07 * math.cos(8 * a)              # bong hoa 8 canh
            if r < petal:
                g = 0.74 if r > petal - 0.03 or r < 0.045 else 0.86
            cu, cv = (u + 0.5) % 1.0 - 0.5, (v + 0.5) % 1.0 - 0.5  # giao diem cua luoi thoi
            if math.hypot(cu, cv) < 0.035:
                g = 0.76
            px += [clamp8(255 * g)] * 3
    return px


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    jobs = (("san_go.png", wood, SIZE, SIZE), ("da_cam_thach.png", marble, SIZE, SIZE),
            ("dong_son.png", dong_son, DS_W, DS_H), ("gom_hoa_lam.png", hoa_lam, HL, HL),
            ("hoa_van_tuong.png", hoa_van_tuong, HV, HV))
    for name, gen, w, h in jobs:
        path = os.path.normpath(os.path.join(OUT, name))
        write_png(path, gen(), w, h)
        print("da tao", path)
