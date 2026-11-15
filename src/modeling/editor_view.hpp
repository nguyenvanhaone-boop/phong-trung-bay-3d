#pragma once
// PHAN A — GIAO DIEN TRINH SOAN DUONG CONG (Khang, DHMTPTB3-16, -17, -27).
// Ve tat ca vao Framebuffer bang Bresenham cua nhom (khong dung GL_LINES).
//   Nua trai : luoi 10 cm, truc quay, da giac dieu khien, duong cong, cau truc de Casteljau.
//   Nua phai : mat tron xoay sinh tu duong cong (Phong), xoay cham, co the hien phap tuyen.
#include <vector>

#include "curve_editor.hpp"
#include "framebuffer.hpp"
#include "pipeline.hpp"

struct EditorLayout {
    int panelWidth = 480;   // be rong nua trai (pixel framebuffer)
    float originX = 60;     // pixel cua diem (0, 0) (goc truc quay, mat day)
    float originY = 500;
    float scale = 600;      // pixel / met

    Vec3 toPixel(Vec3 p) const { return {originX + p.x * scale, originY - p.y * scale, 0}; }
    Vec3 toProfile(float px, float py) const { return {(px - originX) / scale, (originY - py) / scale, 0}; }
};

struct EditorFrame {
    int selected = -1;          // diem dang chon / keo
    float deCasteljau = -1;     // < 0: tat; >= 0: tham so toan cuc g (doan floor(g), t = g - floor(g))
    float previewAngle = 0;     // goc xoay mat tron xoay ben phai (radian)
    bool showNormals = false;
    ShadingMode shading = ShadingMode::Phong;
};

// Ve ca man hinh trinh soan. `preview` la luoi revolve() cua duong cong hien tai (main giu va chi
// tinh lai khi duong cong doi).
void drawEditor(Framebuffer& fb, const CurveEditor& editor, const Mesh& preview, const EditorLayout& layout,
                const EditorFrame& frame);
