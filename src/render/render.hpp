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
};
void fillTriangle(Framebuffer& fb, const ScreenVertex tri[3], const ShadeContext& ctx, const RenderSettings& s);

// Ve doan thang bang Bresenham (Lab 01) — Khang cai, dung cho che do khung day.
void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Vec3 color);

// Ve ca canh vao framebuffer. Thu tu: bien doi -> xen mat gan -> chieu -> Back-face -> Painter/Z-buffer -> to.
void renderScene(Framebuffer& fb, const std::vector<SceneObject>& objects, const Camera& camera,
                 const std::vector<Light>& lights, const RenderSettings& settings, FrameStats& stats);
