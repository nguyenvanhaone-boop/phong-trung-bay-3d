// checks.cpp — kiem tra nhanh cac thuat toan, chay bang `make check`.
// Moi nguoi them test cho phan minh khi cai xong thuat toan (bo comment cac dong "SAU KHI CAI").
#include <cmath>
#include <cstdio>
#include <string>

#include "framebuffer.hpp"
#include "math3d.hpp"
#include "mesh.hpp"
#include "modeling/modeling.hpp"
#include "render/render.hpp"
#include "scene/scene.hpp"

static int gFail = 0, gPass = 0;

static void expect(bool ok, const std::string& what) {
    if (ok) { ++gPass; } else { ++gFail; std::printf("  SAI: %s\n", what.c_str()); }
}
static bool near(Vec3 a, Vec3 b, float eps = 1e-4f) { return length(a - b) < eps; }

int main() {
    // --- Khung chung (da cai) ---
    expect(near(transformPoint(translate({1, 2, 3}), {0, 0, 0}), {1, 2, 3}), "translate");
    expect(near(transformPoint(rotateY(radians(90)), {1, 0, 0}), {0, 0, -1}), "rotateY 90 do: (1,0,0) -> (0,0,-1)");
    expect(near(cross({1, 0, 0}, {0, 1, 0}), {0, 0, 1}), "cross x * y = z");

    Framebuffer fb;
    fb.resize(4, 3);
    fb.setPixel(0, 0, {1, 0, 0});
    expect(fb.color[fb.index(0, 0) * 4] == 255 && fb.index(0, 0) == 8, "pixel (0,0) o goc tren trai = hang luu cuoi");

    // --- A · Khang: SAU KHI CAI de Casteljau ---
    // std::array<Vec3, 4> P = {{{0, 0, 0}, {1, 2, 0}, {3, 2, 0}, {4, 0, 0}}};
    // expect(near(bezierPoint(P, 0.5f), {2, 1.5f, 0}), "Bezier tai t = 0.5");

    // --- B · Hao: SAU KHI CAI look-at ---
    // Camera cam; cam.eye = {0, 0, 5}; cam.target = {0, 0, 0};
    // expect(near(transformPoint(cam.viewMatrix(), {0, 0, 0}), {0, 0, -5}), "goc toa do nam truoc camera 5 don vi");

    // --- C · Lam: SAU KHI CAI to dong quet ---
    // dem so pixel duoc to cua mot tam giac vuong 10x10 ~ 50.

    std::printf("%d dung, %d sai\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
