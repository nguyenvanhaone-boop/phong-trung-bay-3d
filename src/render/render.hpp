#pragma once
// PHAN C — HIEN THI (Lam). Slide CG.3 (to dong quet), CG.6 (khu mat khuat, to bong), Lab 06-07.
#include <vector>

#include "framebuffer.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "../scene/scene.hpp"

// Cong thuc chieu sang Phong (hoac Blinn-Phong) tai mot diem. Dung chung cho Flat, Gouraud, Phong.
Vec3 shadePhong(Vec3 point, Vec3 normal, Vec3 eye, const std::vector<Light>& lights,
                const Material& material, bool blinn);

// Back-face: true neu tam giac quay lung ve mat (bo qua, khong ve).
bool isBackFace(const ScreenVertex tri[3]);

// To mot tam giac theo dong quet (slide CG.3), co Z-buffer neu settings.zBuffer.
// Flat: dung tri[0].color cho ca tam giac. Gouraud: noi suy color. Phong: noi suy normal, world roi goi shadePhong.
struct ShadeContext {
    const std::vector<Light>* lights = nullptr;
    const Material* material = nullptr;
    Vec3 eye;
    bool perspective = true;  // camera chieu phoi canh? (song song thi noi suy tuyen tinh tren man hinh da dung)
};

// Trong so noi suy tai diem (px, py) tren man hinh (Sprint 5, DHMTPTB3-41).
// Thuoc tinh A tai diem do = w[0] A0 + w[1] A1 + w[2] A2.
//   perspective = false: toa do trong tam (b0, b1, b2) tren man hinh -> dung cho chieu song song.
//   perspective = true : chieu phoi canh lam "co" tam giac theo 1/z, nen noi suy tuyen tinh tren
//                        man hinh la SAI. Noi suy A/z va 1/z (deu tuyen tinh tren man hinh) roi chia:
//                        w_i = (b_i / z_i) / (b0/z0 + b1/z1 + b2/z2)
// Lam co the dung lai cho Gouraud (color), Phong (normal, world) va Z-buffer.
void interpolationWeights(const ScreenVertex tri[3], float px, float py, bool perspective, float w[3]);
void fillTriangle(Framebuffer& fb, const ScreenVertex tri[3], const ShadeContext& ctx, const RenderSettings& s);

// Ve doan thang bang Bresenham (Lab 01) — Khang cai, dung cho che do khung day.
void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Vec3 color);

// Cat doan thang theo khung [0, w-1] x [0, h-1] (Liang–Barsky). false = nam ngoai hoan toan.
bool clipLineToRect(float& ax, float& ay, float& bx, float& by, float w, float h);

// Ve ca canh vao framebuffer. Thu tu: bien doi -> xen mat gan -> chieu -> Back-face -> Painter/Z-buffer -> to.
void renderScene(Framebuffer& fb, const std::vector<SceneObject>& objects, const Camera& camera,
                 const std::vector<Light>& lights, const RenderSettings& settings, FrameStats& stats);
