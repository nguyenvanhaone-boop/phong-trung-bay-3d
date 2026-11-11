// checks.cpp — kiem tra nhanh cac thuat toan, chay bang `make check`.
// Moi nguoi them test cho phan minh khi cai xong thuat toan (bo comment cac dong "SAU KHI CAI").
#include <cmath>
#include <cstdio>
#include <string>
#include <algorithm>
#include <limits>

#include "framebuffer.hpp"
#include "math3d.hpp"
#include "mesh.hpp"
#include "modeling/modeling.hpp"
#include "render/render.hpp"
#include "scene/scene.hpp"
#include "platform/frame_counter.hpp"

static int gFail = 0, gPass = 0;

static void expect(bool ok, const std::string& what) {
    if (ok) { ++gPass; } else { ++gFail; std::printf("  SAI: %s\n", what.c_str()); }
}
static bool near(Vec3 a, Vec3 b, float eps = 1e-4f) { return length(a - b) < eps; }

static int coloredPixels(const Framebuffer& framebuffer) {
    int count = 0;
    for (std::size_t offset = 0; offset < framebuffer.color.size(); offset += 4)
        if (framebuffer.color[offset] || framebuffer.color[offset + 1] || framebuffer.color[offset + 2]) ++count;
    return count;
}

static ScreenVertex screenVertex(float horizontal, float vertical) {
    ScreenVertex vertex;
    vertex.x = horizontal;
    vertex.y = vertical;
    vertex.color = {1, 0, 0};
    return vertex;
}

static void checkDisplay() {
    Framebuffer framebuffer;
    framebuffer.resize(4, 3);
    framebuffer.clear({-1, 0.5f, 2});
    bool clearCorrect = true;
    for (std::size_t offset = 0; offset < framebuffer.color.size(); offset += 4)
        clearCorrect = clearCorrect && framebuffer.color[offset] == 0 && framebuffer.color[offset + 1] == 128
                     && framebuffer.color[offset + 2] == 255 && framebuffer.color[offset + 3] == 255;
    expect(clearCorrect, "framebuffer clear RGBA va kep mau [0,1]");
    framebuffer.depthAt(1, 1) = 2;
    framebuffer.clear({0, 0, 0});
    expect(std::all_of(framebuffer.depth.begin(), framebuffer.depth.end(), [](float depth) {
        return depth == std::numeric_limits<float>::infinity();
    }), "clear dat lai depth");
    framebuffer.setPixel(0, 0, {1, 0, 0});
    framebuffer.setPixel(0, 2, {0, 1, 0});
    expect(framebuffer.color[32] == 255 && framebuffer.color[1] == 255, "RGBA luu duoi len, toa do logic tren xuong");
    const auto before = framebuffer.color;
    framebuffer.setPixel(-1, 0, {1, 1, 1});
    framebuffer.setPixel(4, 0, {1, 1, 1});
    framebuffer.setPixel(0, -1, {1, 1, 1});
    framebuffer.setPixel(0, 3, {1, 1, 1});
    expect(framebuffer.color == before, "setPixel ngoai khung khong ghi bo nho");
    framebuffer.resize(7, 5);
    expect(framebuffer.width == 7 && framebuffer.height == 5 && framebuffer.color.size() == 140
           && framebuffer.depth.size() == 35, "resize cap phat dung kich thuoc");

    const FrameCounter::Clock::time_point start{};
    FrameCounter counter(start);
    expect(counter.fps() == 0, "FPS khoi tao bang 0");
    for (int frame = 1; frame <= 4; ++frame) counter.record(start + std::chrono::milliseconds(frame * 100));
    expect(counter.fps() == 0, "FPS cho du cua so 0.5 giay");
    counter.record(start + std::chrono::milliseconds(500));
    expect(std::abs(counter.fps() - 10) < 1e-6, "5 frame / 0.5 giay = 10 FPS");
    counter.record(start + std::chrono::milliseconds(1500));
    expect(std::abs(counter.fps() - 1) < 1e-6, "FPS reset bo dem sau moi cua so");
    FrameCounter fastCounter(start);
    for (int frame = 1; frame <= 500; ++frame)
        fastCounter.record(start + std::chrono::milliseconds(frame));
    expect(std::abs(fastCounter.fps() - 1000) < 1e-6, "FPS khong bi kep o 60");
}

static void checkScanline() {
    Framebuffer framebuffer;
    framebuffer.resize(16, 16);
    RenderSettings settings;
    settings.zBuffer = false;
    const ShadeContext context;
    ScreenVertex triangle[3] = {screenVertex(2, 2), screenVertex(12, 2), screenVertex(2, 12)};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, triangle, context, settings);
    expect(coloredPixels(framebuffer) == 45, "tam giac vuong 10x10: 45 tam pixel theo bien nua mo");
    const auto reference = framebuffer.color;
    for (ShadingMode mode : {ShadingMode::Gouraud, ShadingMode::Phong}) {
        settings.shading = mode;
        framebuffer.clear({0, 0, 0});
        fillTriangle(framebuffer, triangle, context, settings);
        expect(framebuffer.color == reference, "che do chua ho tro fallback mau phang thay vi bo ve");
    }
    settings.shading = ShadingMode::Flat;
    int order[3] = {0, 1, 2};
    bool sameCoverage = true;
    do {
        ScreenVertex permuted[3] = {triangle[order[0]], triangle[order[1]], triangle[order[2]]};
        framebuffer.clear({0, 0, 0});
        fillTriangle(framebuffer, permuted, context, settings);
        sameCoverage = sameCoverage && framebuffer.color == reference;
    } while (std::next_permutation(order, order + 3));
    expect(sameCoverage, "6 hoan vi dinh cho cung vung to");

    triangle[0].color = {0, 1, 0};
    triangle[1].color = {0, 0, 1};
    triangle[2].color = {1, 0, 0};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, triangle, context, settings);
    const std::size_t inside = framebuffer.index(3, 3) * 4;
    expect(framebuffer.color[inside] == 0 && framebuffer.color[inside + 1] == 255
           && framebuffer.color[inside + 2] == 0, "mau phang lay tu dinh dau vao dau tien");

    ScreenVertex other[3] = {screenVertex(12, 2), screenVertex(12, 12), screenVertex(2, 12)};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, other, context, settings);
    int overlap = 0;
    for (std::size_t offset = 0; offset < reference.size(); offset += 4)
        if (reference[offset] && framebuffer.color[offset]) ++overlap;
    expect(overlap == 0 && coloredPixels(framebuffer) + 45 == 100, "2 tam giac ghep vuong: khong ho, khong trung canh");

    ScreenVertex general[3] = {screenVertex(2, 1), screenVertex(12, 6), screenVertex(2, 11)};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, general, context, settings);
    expect(coloredPixels(framebuffer) == 50, "tam giac 3 cao do khac nhau: doi canh giua");
    const auto generalReference = framebuffer.color;
    int generalOrder[3] = {0, 1, 2};
    bool sameSplitCoverage = true;
    do {
        ScreenVertex permuted[3] = {general[generalOrder[0]], general[generalOrder[1]], general[generalOrder[2]]};
        framebuffer.clear({0, 0, 0});
        fillTriangle(framebuffer, permuted, context, settings);
        sameSplitCoverage = sameSplitCoverage && framebuffer.color == generalReference;
    } while (std::next_permutation(generalOrder, generalOrder + 3));
    expect(sameSplitCoverage, "chia hai nua khong phu thuoc thu tu dinh");
    ScreenVertex splitAtCenter[3] = {screenVertex(2, 1.5f), screenVertex(12, 6.5f), screenVertex(2, 11.5f)};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, splitAtCenter, context, settings);
    expect(coloredPixels(framebuffer) == 50, "dong qua dinh giua chi thuoc nua duoi");
    ScreenVertex fractional[3] = {screenVertex(2.25f, 2.25f), screenVertex(12.25f, 2.25f), screenVertex(2.25f, 12.25f)};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, fractional, context, settings);
    expect(coloredPixels(framebuffer) == 55, "toa do le lay mau tai tam pixel");

    ScreenVertex covering[3] = {screenVertex(-100, -100), screenVertex(200, -100), screenVertex(-100, 200)};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, covering, context, settings);
    expect(coloredPixels(framebuffer) == 256, "kep dong va cot vao framebuffer");
    ScreenVertex outside[3] = {screenVertex(-20, -20), screenVertex(-10, -20), screenVertex(-20, -10)};
    framebuffer.clear({0, 0, 0});
    fillTriangle(framebuffer, outside, context, settings);
    expect(coloredPixels(framebuffer) == 0, "tam giac ngoai man hinh");
    ScreenVertex degenerate[3] = {screenVertex(2, 2), screenVertex(5, 5), screenVertex(10, 10)};
    fillTriangle(framebuffer, degenerate, context, settings);
    expect(coloredPixels(framebuffer) == 0, "bo qua tam giac thang hang");
    degenerate[2] = degenerate[0];
    fillTriangle(framebuffer, degenerate, context, settings);
    expect(coloredPixels(framebuffer) == 0, "bo qua dinh trung nhau");
    degenerate[0].x = std::numeric_limits<float>::quiet_NaN();
    fillTriangle(framebuffer, degenerate, context, settings);
    expect(coloredPixels(framebuffer) == 0, "bo qua toa do NaN");
    degenerate[0].x = std::numeric_limits<float>::infinity();
    fillTriangle(framebuffer, degenerate, context, settings);
    expect(coloredPixels(framebuffer) == 0, "bo qua toa do vo cuc");
    Framebuffer empty;
    fillTriangle(empty, triangle, context, settings);
    expect(empty.color.empty(), "framebuffer rong khong gay loi");

    SceneObject object;
    object.mesh.vertices = {{{-0.5f, -0.5f, -1}, {0, 0, 1}},
                            {{0.5f, -0.5f, -1}, {0, 0, 1}},
                            {{0, 0.5f, -1}, {0, 0, 1}}};
    object.mesh.triangles = {{0, 1, 2}};
    object.mesh.material.kd = {0.25f, 0.75f, 0.5f};
    settings.backFace = false;
    FrameStats stats;
    std::vector<std::uint8_t> flatScene;
    for (ShadingMode mode : {ShadingMode::Flat, ShadingMode::Gouraud, ShadingMode::Phong}) {
        settings.shading = mode;
        framebuffer.clear({0, 0, 0});
        renderScene(framebuffer, {object}, Camera{}, {Light{}}, settings, stats);
        if (mode == ShadingMode::Flat) flatScene = framebuffer.color;
        expect(coloredPixels(framebuffer) > 0 && framebuffer.color == flatScene,
               "renderScene khoi tao mau cho ca Flat/Gouraud/Phong fallback");
    }
}

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

    checkDisplay();
    checkScanline();

    std::printf("%d dung, %d sai\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
