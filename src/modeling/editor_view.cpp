// PHAN A — GIAO DIEN TRINH SOAN DUONG CONG (Khang, DHMTPTB3-16, -17, -27).
#include "editor_view.hpp"

#include <algorithm>
#include <cmath>

#include "render/render.hpp"
#include "render/shadow.hpp"
#include "scene/scene.hpp"

namespace {

const Vec3 kGrid{0.20f, 0.21f, 0.25f};
const Vec3 kAxis{0.85f, 0.35f, 0.35f};
const Vec3 kPolygon{0.45f, 0.47f, 0.55f};
const Vec3 kCurve{1.0f, 0.85f, 0.3f};
const Vec3 kJoint{0.3f, 0.85f, 1.0f};
const Vec3 kHandle{0.95f, 0.95f, 0.95f};
const Vec3 kSelected{1.0f, 0.3f, 0.3f};
const Vec3 kLevel1{0.4f, 0.9f, 0.4f};
const Vec3 kLevel2{0.4f, 0.6f, 1.0f};
const Vec3 kTrace{1.0f, 0.45f, 0.1f};

// Doan thang (toa do pixel float) -> cat theo khung -> Bresenham.
void line(Framebuffer& fb, Vec3 a, Vec3 b, Vec3 color, int maxX) {
    float ax = a.x, ay = a.y, bx = b.x, by = b.y;
    if (clipLineToRect(ax, ay, bx, by, float(maxX), float(fb.height)))
        drawLineBresenham(fb, int(std::lround(ax)), int(std::lround(ay)), int(std::lround(bx)), int(std::lround(by)), color);
}

void square(Framebuffer& fb, Vec3 c, int r, Vec3 color) {
    for (int y = int(c.y) - r; y <= int(c.y) + r; ++y)
        for (int x = int(c.x) - r; x <= int(c.x) + r; ++x) fb.setPixel(x, y, color);
}

void disc(Framebuffer& fb, Vec3 c, int r, Vec3 color) {
    for (int y = -r; y <= r; ++y)
        for (int x = -r; x <= r; ++x)
            if (x * x + y * y <= r * r) fb.setPixel(int(c.x) + x, int(c.y) + y, color);
}

// Ve 1 doan Bezier bang N doan thang nho (lay mau bang de Casteljau cua phan A).
void bezier(Framebuffer& fb, const std::array<Vec3, 4>& P, const EditorLayout& L, Vec3 color, float tEnd = 1) {
    const int steps = 48;
    Vec3 prev = L.toPixel(bezierPoint(P, 0));
    for (int i = 1; i <= steps; ++i) {
        const float t = tEnd * i / steps;
        const Vec3 cur = L.toPixel(bezierPoint(P, t));
        line(fb, prev, cur, color, L.panelWidth);
        line(fb, prev + Vec3{0, 1, 0}, cur + Vec3{0, 1, 0}, color, L.panelWidth);  // net day 2 pixel
        prev = cur;
    }
}

}  // namespace

void drawEditor(Framebuffer& fb, const CurveEditor& editor, const Mesh& preview, const EditorLayout& L,
                const EditorFrame& frame) {
    // ---- Nua trai: mat phang profile ----
    for (int y = 0; y < fb.height; ++y)
        for (int x = 0; x < L.panelWidth; ++x) fb.setPixel(x, y, {0.11f, 0.11f, 0.13f});
    for (int k = 0; k <= 10; ++k) {  // luoi 10 cm
        const float m = k * 0.1f;
        line(fb, L.toPixel({m, 0, 0}), L.toPixel({m, 0.85f, 0}), kGrid, L.panelWidth);
        line(fb, L.toPixel({0, m, 0}), L.toPixel({0.8f, m, 0}), kGrid, L.panelWidth);
    }
    line(fb, L.toPixel({0, -0.05f, 0}), L.toPixel({0, 0.85f, 0}), kAxis, L.panelWidth);  // truc quay y
    line(fb, L.toPixel({0.002f, -0.05f, 0}), L.toPixel({0.002f, 0.85f, 0}), kAxis, L.panelWidth);

    const auto& pts = editor.points;
    for (std::size_t i = 1; i < pts.size(); ++i) line(fb, L.toPixel(pts[i - 1]), L.toPixel(pts[i]), kPolygon, L.panelWidth);

    const Profile pr = editor.toProfile();
    for (const auto& seg : pr.segments) bezier(fb, seg, L, kCurve);

    // de Casteljau (phim T): doan dang chay, 3 doan muc 1, 2 doan muc 2, diem tren cong.
    if (frame.deCasteljau >= 0 && !pr.segments.empty()) {
        const int s = std::min(static_cast<int>(frame.deCasteljau), static_cast<int>(pr.segments.size()) - 1);
        const float t = std::clamp(frame.deCasteljau - s, 0.0f, 1.0f);
        for (int k = 0; k < s; ++k) bezier(fb, pr.segments[k], L, kTrace);
        bezier(fb, pr.segments[s], L, kTrace, t);
        const auto& P = pr.segments[s];
        for (int k = 0; k < 3; ++k) line(fb, L.toPixel(P[k]), L.toPixel(P[k + 1]), kPolygon, L.panelWidth);
        const std::vector<Vec3> lv = deCasteljauLevels(P, t);
        line(fb, L.toPixel(lv[0]), L.toPixel(lv[1]), kLevel1, L.panelWidth);
        line(fb, L.toPixel(lv[1]), L.toPixel(lv[2]), kLevel1, L.panelWidth);
        line(fb, L.toPixel(lv[3]), L.toPixel(lv[4]), kLevel2, L.panelWidth);
        for (int k = 0; k < 3; ++k) disc(fb, L.toPixel(lv[k]), 3, kLevel1);
        for (int k = 3; k < 5; ++k) disc(fb, L.toPixel(lv[k]), 3, kLevel2);
        disc(fb, L.toPixel(lv[5]), 5, kSelected);
    }

    for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
        const Vec3 c = L.toPixel(pts[i]);
        const Vec3 col = i == frame.selected ? kSelected : (editor.kind == CurveEditor::Kind::Bezier && editor.isJoint(i) ? kJoint : kHandle);
        if (editor.kind == CurveEditor::Kind::Bezier && editor.isJoint(i)) square(fb, c, 4, col);
        else disc(fb, c, 4, col);
    }
    for (int y = 0; y < fb.height; ++y) fb.setPixel(L.panelWidth, y, {0.35f, 0.35f, 0.4f});  // vach ngan

    // ---- Nua phai: mat tron xoay (dung lai renderScene cua phan C) ----
    Framebuffer view;
    view.resize(fb.width - L.panelWidth - 1, fb.height);
    view.clear({0.15f, 0.15f, 0.18f});
    if (!preview.triangles.empty()) {
        float top = 0;
        for (const Vertex& v : preview.vertices) top = std::max(top, v.position.y);
        SceneObject obj;
        obj.mesh = preview;
        Camera cam;
        cam.mode = Camera::Mode::Orbit;
        cam.target = {0, top * 0.5f, 0};
        cam.orbitRadius = std::max(0.6f, top * 2.2f);
        cam.orbitPhi = 0.35f;
        cam.orbitTheta = frame.previewAngle;
        const std::vector<Light> lights = {Light{{1.5f, 2.0f, 2.0f}, {0.9f, 0.9f, 0.9f}}, Light{{-2.0f, 1.0f, -1.0f}, {0.3f, 0.3f, 0.35f}}};
        RenderSettings rs;
        rs.shading = frame.shading;
        FrameStats st;
        const std::vector<SceneObject> objs = {obj};
        renderScene(view, objs, cam, lights, rs, st);
        if (frame.showNormals) drawNormals(view, objs, 0, cam, std::max(0.02f, top * 0.06f), {0.3f, 1.0f, 0.5f});
    }
    for (int y = 0; y < view.height; ++y)
        for (int x = 0; x < view.width; ++x) {
            const std::size_t s = view.index(x, y) * 4, d = fb.index(L.panelWidth + 1 + x, y) * 4;
            for (int c = 0; c < 4; ++c) fb.color[d + c] = view.color[s + c];
        }
}
