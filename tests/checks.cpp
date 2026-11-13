// checks.cpp — kiem tra nhanh cac thuat toan, chay bang `make check`.
// Moi nguoi them test cho phan minh khi cai xong thuat toan (bo comment cac dong "SAU KHI CAI").
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>
#include <algorithm>
#include <limits>

#include "framebuffer.hpp"
#include "math3d.hpp"
#include "mesh.hpp"
#include "modeling/modeling.hpp"
#include "render/render.hpp"
#include "scene/obj_loader.hpp"
#include "scene/texture_loader.hpp"
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

    // --- B · Hao: camera ---
    {
        Camera cam;                       // di bo, yaw = pitch = 0: nhin theo -z
        cam.eye = {0, 0, 5};
        const Mat4 V = cam.viewMatrix();
        expect(near(transformPoint(V, {0, 0, 0}), {0, 0, -5}), "look-at: goc toa do nam truoc mat 5 m");
        expect(near(transformPoint(V, cam.eye), {0, 0, 0}), "look-at: mat ve goc he quan sat");
        expect(near(transformPoint(V, {1, 0, 5}), {1, 0, 0}), "look-at: ben phai the gioi = +x camera");

        cam.yaw = radians(90);            // quay phai 90 do: nhin theo +x
        expect(near(cam.forward(), {1, 0, 0}), "yaw 90 do nhin theo +x");
        expect(near(transformPoint(cam.viewMatrix(), {3, 0, 5}), {0, 0, -3}), "look-at sau khi quay: vat o +x nam truoc mat");

        cam.mode = Camera::Mode::Orbit;   // orbit theta = phi = 0: mat o +z cua target
        cam.target = {1, 1, 1}; cam.orbitRadius = 2; cam.orbitTheta = 0; cam.orbitPhi = 0;
        expect(near(cam.eyePosition(), {1, 1, 3}), "orbit: mat cach target 2 m theo +z");
        expect(near(transformPoint(cam.viewMatrix(), cam.target), {0, 0, -2}), "orbit: target nam giua tam nhin");
        cam.zoom(0.5f);
        expect(std::fabs(length(cam.eyePosition() - cam.target) - 1) < 1e-5f, "orbit zoom: ban kinh 2 -> 1, van nhin vao target");
        for (int i = 0; i < 50; ++i) cam.zoom(0.5f);
        expect(cam.orbitRadius >= 0.8f, "orbit zoom: khong lai xuyen vao vat (ban kinh >= 0,8 m)");
        cam.orbitRadius = 2;

        // Phoi canh: diem tren mat phang gan -> z_ndc = -1, tren mat phang xa -> +1.
        Camera p; p.eye = {0, 0, 0};
        const Mat4 P = p.projectionMatrix(16.0f / 9.0f);
        const Vec4 cn = P * Vec4{0, 0, -p.nearPlane, 1}, cf = P * Vec4{0, 0, -p.farPlane, 1};
        expect(std::fabs(cn.z / cn.w + 1) < 1e-3f && std::fabs(cf.z / cf.w - 1) < 1e-3f, "phoi canh: near -> -1, far -> +1");
        // Diem o canh tren cua goc nhin (tan(fovy/2) * d) phai ra y_ndc = 1.
        const float d = 4, yTop = std::tan(radians(p.fovyDeg) * 0.5f) * d;
        const Vec4 ct = P * Vec4{0, yTop, -d, 1};
        expect(std::fabs(ct.y / ct.w - 1) < 1e-4f, "phoi canh: canh tren goc nhin -> y_ndc = 1");
        p.projection = Projection::Parallel;
        const Mat4 O = p.projectionMatrix(1.0f);
        const Vec4 o1 = O * Vec4{0.5f, 0, -2, 1}, o2 = O * Vec4{0.5f, 0, -20, 1};
        expect(std::fabs(o1.x - o2.x) < 1e-5f && o1.w == 1.0f, "song song: x khong doi theo do sau");

        // Kep trong phong: di thang 100 m van o trong phong.
        Camera w; w.walk(100, 0);
        expect(w.eye.z >= -kRoomHalfZ && w.eye.y == kEyeHeight, "di bo: kep trong phong, giu chieu cao mat");
        w.rotate(0, radians(200));
        expect(w.pitch <= radians(89.0f) + 1e-6f, "pitch bi gioi han 89 do");
    }
    {
        // Hop: hop le, phap tuyen mat ra ngoai (hoac vao trong khi inward), u x v dung chieu.
        Material m;
        const Mesh out = makeBox({1, 2, 3}, m), in = makeBox({1, 2, 3}, m, true);
        std::string err;
        expect(validate(out, err) && out.triangles.size() == 12 && out.vertices.size() == 24, "hop: 24 dinh, 12 tam giac");
        bool outOk = true, inOk = true;
        for (const auto& t : out.triangles) {
            const Vec3 c = (out.vertices[t[0]].position + out.vertices[t[1]].position + out.vertices[t[2]].position) / 3.0f;
            const Vec3 n = faceNormal(out, t);
            outOk = outOk && dot(n, c) > 0 && dot(n, out.vertices[t[0]].normal) > 0;
        }
        for (const auto& t : in.triangles) {
            const Vec3 c = (in.vertices[t[0]].position + in.vertices[t[1]].position + in.vertices[t[2]].position) / 3.0f;
            const Vec3 n = faceNormal(in, t);
            inOk = inOk && dot(n, c) < 0 && dot(n, in.vertices[t[0]].normal) > 0;
        }
        expect(outOk, "hop: chieu dinh nguoc kim dong ho, phap tuyen huong ra ngoai");
        expect(inOk, "hop (inward): phap tuyen huong vao trong phong");

        const std::vector<SceneObject> room = buildRoom();
        bool allValid = true;
        int tris = 0;
        for (const SceneObject& o : room) { allValid = allValid && validate(o.mesh, err); tris += static_cast<int>(o.mesh.triangles.size()); }
        expect(room.size() == 13 && allValid, "phong: san, tuong, tran + 5 buc + 5 vat, luoi hop le");
        std::printf("  phong co %zu vat, %d tam giac\n", room.size(), tris);
    }
    {
        // Xen mat gan (Sutherland–Hodgman), near = 1: "trong" la z <= -1.
        auto vtx = [](float x, float y, float z, float u) {
            ViewVertex v;
            v.view = {x, y, z}; v.world = {x, y, z}; v.normal = {0, 0, 1}; v.u = u; v.v = 0;
            return v;
        };
        const float n = 1;
        const ViewVertex allIn[3] = {vtx(0, 0, -2, 0), vtx(1, 0, -2, 0), vtx(0, 1, -3, 0)};
        const ViewVertex allOut[3] = {vtx(0, 0, 0.5f, 0), vtx(1, 0, -0.5f, 0), vtx(0, 1, 2, 0)};
        expect(clipNear(allIn, n).size() == 3, "xen: tam giac nam truoc mat phang gan giu nguyen");
        expect(clipNear(allOut, n).empty(), "xen: tam giac nam sau mat phang gan bi bo");

        // 1 dinh trong (z = -3), 2 dinh ngoai (z = 1): ra tam giac nho hon, 3 dinh.
        const ViewVertex one[3] = {vtx(0, 0, -3, 0), vtx(2, 0, 1, 1), vtx(0, 2, 1, 1)};
        const std::vector<ViewVertex> p1 = clipNear(one, n);
        bool ok1 = p1.size() == 3;
        for (const ViewVertex& v : p1) ok1 = ok1 && v.view.z <= -n + 1e-6f;
        // Canh (0,0,-3) -> (2,0,1) cat z = -1 tai t = 0.5: diem (1, 0, -1), u = 0.5.
        bool hit = false;
        for (const ViewVertex& v : p1) hit = hit || (near(v.view, {1, 0, -1}) && std::fabs(v.u - 0.5f) < 1e-5f);
        expect(ok1 && hit, "xen 1 dinh trong: 3 dinh, giao diem dung, u noi suy cung t");

        // 2 dinh trong, 1 dinh ngoai: ra tu giac 4 dinh, giu chieu nguoc kim dong ho.
        const ViewVertex two[3] = {vtx(0, 0, -3, 0), vtx(2, 0, -3, 0), vtx(1, 2, 1, 0)};
        const std::vector<ViewVertex> p2 = clipNear(two, n);
        bool ok2 = p2.size() == 4;
        for (const ViewVertex& v : p2) ok2 = ok2 && v.view.z <= -n + 1e-6f;
        expect(ok2, "xen 2 dinh trong: tu giac 4 dinh, khong dinh nao sau mat phang gan");
        if (p2.size() == 4) {
            const Vec3 nBefore = cross(two[1].view - two[0].view, two[2].view - two[0].view);
            bool sameWinding = true;
            for (std::size_t k = 1; k + 1 < p2.size(); ++k)
                sameWinding = sameWinding && dot(cross(p2[k].view - p2[0].view, p2[k + 1].view - p2[0].view), nBefore) > 0;
            expect(sameWinding, "xen: cac tam giac quat giu chieu dinh (back-face van dung)");
        }
        // Dinh nam dung tren mat phang gan duoc coi la "trong", khong sinh dinh trung.
        const ViewVertex onPlane[3] = {vtx(0, 0, -1, 0), vtx(1, 0, -2, 0), vtx(0, 1, -2, 0)};
        expect(clipNear(onPlane, n).size() == 3, "xen: dinh nam dung tren mat phang gan giu nguyen");
    }

    {
        // Chon vat bang tia (DHMTPTB3-36).
        const Mat4 M = translate({1, 2, 3}) * rotateY(radians(30)) * scale({2, 1, 0.5f});
        Mat4 Mi;
        const bool okInv = inverse(M, Mi);
        const Mat4 I = M * Mi;
        bool isId = okInv;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j) isId = isId && std::fabs(I.m[i][j] - (i == j ? 1.0f : 0.0f)) < 1e-5f;
        expect(isId, "nghich dao: M * M^-1 = I");
        Mat4 dummy;
        expect(!inverse(scale({1, 0, 1}), dummy), "nghich dao: ma tran suy bien tra ve false");

        // Pixel giua man hinh -> tia trung voi huong nhin, ca phoi canh lan song song.
        Camera cam;
        cam.eye = {0, 1.6f, 4};
        const Ray r = rayFromPixel(cam, 480, 270, 960, 540, 960.0f / 540);
        expect(near(r.dir, cam.forward(), 1e-3f) && length(cross(r.origin - cam.eye, cam.forward())) < 1e-3f,
               "tia qua tam man hinh: di theo huong nhin, xuat phat tu mat");
        cam.projection = Projection::Parallel;
        const Ray r1 = rayFromPixel(cam, 100, 100, 960, 540, 960.0f / 540);
        const Ray r2 = rayFromPixel(cam, 800, 400, 960, 540, 960.0f / 540);
        expect(near(r1.dir, r2.dir, 1e-3f) && near(r1.dir, cam.forward(), 1e-3f), "chieu song song: moi tia song song voi huong nhin");
        cam.projection = Projection::Perspective;

        // Hai hop tren cung mot duong nhin: chon hop gan hon; hop xa duoc dat bang ma tran co ti le.
        std::vector<SceneObject> objs(2);
        objs[0].mesh = makeBox({0.5f, 0.5f, 0.5f}, Material{});
        objs[0].model = translate({0, 1.6f, -6}) * scale({2, 2, 2});
        objs[1].mesh = makeBox({0.5f, 0.5f, 0.5f}, Material{});
        objs[1].model = translate({0, 1.6f, 0}) * rotateY(radians(45));
        const PickResult hit = pick(objs, r);
        expect(hit.index == 1, "chon: trung hop gan hon khi 2 hop chong len nhau");
        // Hop gan quay 45 do: canh dung cua hop huong ve mat, nam o z = 0,5*sqrt(2).
        // (Goc tia nam tren mat phang gan, cach mat 0,1 m, nen so diem trung thay vi t.)
        expect(near(hit.point, {0, 1.6f, 0.5f * std::sqrt(2.0f)}, 1e-3f), "chon: diem trung dung (qua ma tran nghich dao)");
        objs[1].visible = false;
        const PickResult far = pick(objs, r);
        expect(far.index == 0 && near(far.point, {0, 1.6f, -5}, 1e-3f), "chon: hop xa phong to 2 lan, diem trung dung (t khong doi khi doi he toa do)");
        const Ray miss = rayFromPixel(cam, 5, 5, 960, 540, 960.0f / 540);
        expect(pick(objs, miss).index == -1, "chon: click vao khoang trong -> khong chon");

        // Dung trong hop (giong dung trong phong): mat hop quay lung -> khong chan tia.
        objs[1].visible = true;
        Ray inside{{0, 1.6f, 0}, {0, 0, -1}};
        expect(pick(objs, inside).index == 0, "chon: bo qua mat quay lung (dung ben trong hop)");

        // Phong that: click giua man hinh tu cho xuat phat trung tuong cuoi (khong trung tran/san).
        const std::vector<SceneObject> room = buildRoom();
        Camera start;
        const PickResult wall = pick(room, rayFromPixel(start, 480, 270, 960, 540, 960.0f / 540));
        expect(wall.index == 1 && std::fabs(wall.point.z + kRoomHalfZ) < 1e-3f, "phong: tia giua man hinh trung tuong cuoi");
    }

    checkDisplay();
    checkScanline();
    {
        // Nap .obj (DHMTPTB3-42).
        auto parse = [](const std::string& text, Mesh& m, std::string& err) {
            std::istringstream in(text);
            return parseObj(in, m, err);
        };
        Mesh m;
        std::string err;

        // Tu giac "v/vt/vn" -> 2 tam giac, 4 dinh; uv va phap tuyen doc dung.
        const char* quad =
            "# hinh vuong\n"
            "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
            "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
            "vn 0 0 2\n"                       // chua chuan hoa: bo doc phai chuan hoa
            "g mat\nusemtl gom\ns off\n"       // lenh khong ho tro: bo qua
            "f 1/1/1 2/2/1 3/3/1 4/4/1\r\n";   // ket thuc dong kieu Windows
        expect(parse(quad, m, err) && m.triangles.size() == 2 && m.vertices.size() == 4, "obj: tu giac -> 2 tam giac, 4 dinh");
        expect(m.vertices.size() == 4 && near(m.vertices[2].normal, {0, 0, 1}) && m.vertices[2].u == 1 && m.vertices[2].v == 1,
               "obj: doc vt, vn (da chuan hoa)");
        expect(m.triangles.size() == 2 && dot(faceNormal(m, m.triangles[0]), {0, 0, 1}) > 0, "obj: giu chieu nguoc kim dong ho");

        // Chi so am va kieu "v//vn"; cung vi tri nhung khac vn -> 2 Vertex rieng.
        const char* neg =
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nvn 0 0 -1\n"
            "f -3//-2 -2//-2 -1//-2\n"
            "f 1//2 3//2 2//2\n";
        expect(parse(neg, m, err) && m.triangles.size() == 2 && m.vertices.size() == 6, "obj: chi so am, v//vn, tach dinh khac vn");

        // Khong co vn: tinh phap tuyen tu cac mat. Hop lap phuong 8 dinh -> phap tuyen dinh = huong cheo ra ngoai.
        const char* cube =
            "v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\nv -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n"
            "f 5 6 7 8\nf 2 1 4 3\nf 1 5 8 4\nf 6 2 3 7\nf 8 7 3 4\nf 1 2 6 5\n";
        bool cubeOk = parse(cube, m, err) && m.vertices.size() == 8 && m.triangles.size() == 12 && validate(m, err);
        for (const Vertex& v : m.vertices) cubeOk = cubeOk && near(v.normal, normalize(v.position), 1e-3f);
        expect(cubeOk, "obj: khong co vn -> phap tuyen dinh trung binh cac mat, huong ra ngoai");

        // File sai: bao loi kem so dong, khong crash.
        expect(!parse("v 0 0 0\nv 1 0 0\nf 1 2 3\n", m, err) && err.find("dong 3") == 0, "obj: chi so ngoai pham vi -> loi o dong 3");
        expect(!parse("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2\n", m, err), "obj: mat 2 dinh -> loi");
        expect(!parse("v 0 0\n", m, err), "obj: v thieu toa do -> loi");
        expect(!parse("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 x 3\n", m, err), "obj: chi so khong phai so -> loi");
        expect(!parse("v 0 0 0\n", m, err), "obj: khong co mat -> loi");
        expect(!loadObj("khong/ton/tai.obj", m, err), "obj: file khong ton tai -> loi");

        // Am tra Utah (chi co v va f, 6320 tam giac).
        Mesh tea;
        const bool teaOk = loadObj("assets/teapot.obj", tea, err);
        expect(teaOk && tea.name == "teapot" && tea.triangles.size() == 6320 && validate(tea, err), "obj: nap am tra Utah, 6320 tam giac");
        if (teaOk) {
            fitToHeight(tea, 0.45f);
            float lo = 1e9f, hi = -1e9f, volume = 0;
            bool unit = true;
            for (const Vertex& v : tea.vertices) {
                lo = std::min(lo, v.position.y); hi = std::max(hi, v.position.y);
                unit = unit && std::fabs(length(v.normal) - 1) < 1e-3f;
            }
            for (const auto& t : tea.triangles)  // the tich co dau > 0 <=> mat huong ra ngoai
                volume += dot(tea.vertices[t[0]].position, cross(tea.vertices[t[1]].position, tea.vertices[t[2]].position)) / 6;
            expect(std::fabs(lo) < 1e-5f && std::fabs(hi - 0.45f) < 1e-5f, "obj: fitToHeight -> day y = 0, cao 0,45 m");
            expect(unit, "obj: moi phap tuyen dinh do dai 1");
            expect(volume > 0, "obj: am tra dung chieu dinh (phap tuyen ra ngoai)");
        }
        const std::vector<SceneObject> room = buildRoom("assets/teapot.obj");
        expect(room.size() == 15 && room.back().mesh.name == "teapot", "phong: them buc giua + am tra");
        std::string roomErr;
        expect(buildRoom("khong/co.obj", &roomErr).size() == 13 && !roomErr.empty(), "phong: .obj loi -> bo qua, bao ly do");
    }
    {
        // Texture (DHMTPTB3-41).
        // Anh 2 x 2: hang tren (v ~ 1) do, xanh la; hang duoi (v ~ 0) xanh duong, trang.
        Texture t;
        t.width = t.height = 2;
        t.texels = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
        expect(near(t.sample(0.25f, 0.75f), {1, 0, 0}) && near(t.sample(0.75f, 0.25f), {1, 1, 1}),
               "texture: tam texel lay dung mau, v = 0 la hang duoi");
        expect(near(t.sample(1.25f, 0.75f), {1, 0, 0}) && near(t.sample(-0.75f, 2.75f), {1, 0, 0}), "texture: lap lai (repeat) ngoai [0, 1]");
        expect(near(t.sample(0.5f, 0.5f), {0.5f, 0.5f, 0.5f}), "texture: song tuyen o giua 4 texel = trung binh");

        std::string err;
        const auto wood = loadTexture("assets/textures/san_go.png", err);
        expect(wood && wood->width == 256 && wood->height == 256, "texture: nap san_go.png 256 x 256 bang stb_image");
        expect(!loadTexture("khong/co.png", err) && !err.empty(), "texture: file khong ton tai -> null, co ly do");
        const auto checker = makeCheckerTexture(8, 2, {0, 0, 0}, {1, 1, 1});
        expect(near(checker->at(0, 0), {0, 0, 0}) && near(checker->at(4, 0), {1, 1, 1}) && near(checker->at(4, 4), {0, 0, 0}),
               "texture: anh o co");

        // Trong so noi suy: canh tu dinh 0 (z = 1) toi dinh 1 (z = 3), lay trung diem TREN MAN HINH.
        ScreenVertex tri[3];
        tri[0].x = 0; tri[0].y = 0; tri[0].z = 1;
        tri[1].x = 100; tri[1].y = 0; tri[1].z = 3;
        tri[2].x = 0; tri[2].y = 100; tri[2].z = 1;
        float w[3];
        interpolationWeights(tri, 50, 0, false, w);
        expect(std::fabs(w[0] - 0.5f) < 1e-5f && std::fabs(w[1] - 0.5f) < 1e-5f, "noi suy tuyen tinh: trung diem man hinh -> (0,5; 0,5)");
        interpolationWeights(tri, 50, 0, true, w);
        // Dinh 1 xa gap 3 nen bi "nen" lai: trung diem man hinh ung voi 1/4 duong trong 3D.
        expect(std::fabs(w[1] - 0.25f) < 1e-5f && std::fabs(w[0] + w[1] + w[2] - 1) < 1e-5f, "noi suy phoi canh: trung diem man hinh -> 1/4 duong trong 3D");

        // Kiem tra voi phep chieu that: diem P = 0,2 A + 0,3 B + 0,5 C tren san, chieu len man hinh,
        // trong so phoi canh tai pixel cua P phai tra lai dung (0,2; 0,3; 0,5).
        Camera cam;
        cam.eye = {0, 1.6f, 0};
        const int W = 960, H = 540;
        const Mat4 V = cam.viewMatrix(), P = cam.projectionMatrix(static_cast<float>(W) / H);
        auto toScreen = [&](Vec3 world) {
            const Vec3 v = transformPoint(V, world);
            const Vec4 c = P * Vec4{v.x, v.y, v.z, 1};
            ScreenVertex s;
            s.x = (c.x / c.w * 0.5f + 0.5f) * W;
            s.y = (0.5f - c.y / c.w * 0.5f) * H;
            s.z = -v.z;
            return s;
        };
        const Vec3 A{-1, 0, -2}, B{1, 0, -2}, C{0, 0, -8};
        const ScreenVertex floorTri[3] = {toScreen(A), toScreen(B), toScreen(C)};
        const ScreenVertex at = toScreen(A * 0.2f + B * 0.3f + C * 0.5f);
        interpolationWeights(floorTri, at.x, at.y, true, w);
        expect(std::fabs(w[0] - 0.2f) < 1e-3f && std::fabs(w[1] - 0.3f) < 1e-3f && std::fabs(w[2] - 0.5f) < 1e-3f,
               "noi suy phoi canh: khop toa do trong tam 3D cua diem tren san");
        float affine[3];
        interpolationWeights(floorTri, at.x, at.y, false, affine);
        expect(std::fabs(affine[2] - 0.5f) > 0.05f, "noi suy tuyen tinh tren man hinh lech khoi gia tri dung (ly do can phoi canh)");

        // To that: tam giac phu anh o co 2 x 2 o -> co ca o den lan o trang. Tat texture -> mot mau kd.
        Framebuffer fb;
        fb.resize(64, 64);
        Material m;
        m.texture = makeCheckerTexture(8, 2, {0, 0, 0}, {1, 1, 1});
        ScreenVertex big[3];
        big[0].x = 0; big[0].y = 0; big[0].z = 1; big[0].u = 0; big[0].v = 1;
        big[1].x = 64; big[1].y = 0; big[1].z = 1; big[1].u = 1; big[1].v = 1;
        big[2].x = 0; big[2].y = 64; big[2].z = 1; big[2].u = 0; big[2].v = 0;
        big[0].color = {1, 1, 1};
        ShadeContext ctx;
        ctx.material = &m;
        RenderSettings rs;
        auto countDarkLight = [&](int& dark, int& light) {
            dark = light = 0;
            for (int y = 0; y < 64; ++y)
                for (int x = 0; x < 64; ++x) {
                    if (x + y >= 63) continue;  // chi dem phan trong tam giac
                    const int r = fb.color[fb.index(x, y) * 4];
                    if (r < 40) ++dark;
                    if (r > 215) ++light;
                }
        };
        int dark, light;
        fb.clear({0.5f, 0.5f, 0.5f});
        fillTriangle(fb, big, ctx, rs);
        countDarkLight(dark, light);
        expect(dark > 300 && light > 300, "fillTriangle + texture: hien ca o den va o trang");
        rs.texture = false;
        fb.clear({0.5f, 0.5f, 0.5f});
        fillTriangle(fb, big, ctx, rs);
        countDarkLight(dark, light);
        expect(dark == 0 && light > 1500, "fillTriangle tat texture (phim U): chi con mau kd");

        const std::vector<SceneObject> room = buildRoom();
        expect(room[0].mesh.material.texture && room[kRoomShellCount].mesh.material.texture && !room[1].mesh.material.texture,
               "phong: san va buc co texture, tuong khong");
    }

    std::printf("%d dung, %d sai\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
