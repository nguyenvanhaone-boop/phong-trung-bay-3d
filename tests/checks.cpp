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
#include "render/shadow.hpp"
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
    // Gouraud, Phong da cai (DHMTPTB3-25, -31): mau khac Flat nhung phai to DUNG cung tap pixel.
    int flatCoverage = -1;
    for (ShadingMode mode : {ShadingMode::Flat, ShadingMode::Gouraud, ShadingMode::Phong}) {
        settings.shading = mode;
        framebuffer.clear({0, 0, 0});
        renderScene(framebuffer, {object}, Camera{}, {Light{}}, settings, stats);
        if (mode == ShadingMode::Flat) flatCoverage = coloredPixels(framebuffer);
        expect(coloredPixels(framebuffer) > 0 && coloredPixels(framebuffer) == flatCoverage,
               "renderScene: Flat/Gouraud/Phong to cung tap pixel");
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

    // --- A · Khang: Bezier, mat tron xoay ---
    {
        const std::array<Vec3, 4> P = {{{0, 0, 0}, {1, 2, 0}, {3, 2, 0}, {4, 0, 0}}};
        expect(near(bezierPoint(P, 0.5f), {2, 1.5f, 0}), "Bezier tai t = 0.5");
        expect(near(bezierPoint(P, 0), P[0]) && near(bezierPoint(P, 1), P[3]), "Bezier di qua P0 (t = 0) va P3 (t = 1)");
        bool bern = true;
        for (float t : {0.1f, 0.3f, 0.77f}) {  // so voi cong thuc Bernstein
            const float s = 1 - t;
            const Vec3 b = P[0] * (s * s * s) + P[1] * (3 * t * s * s) + P[2] * (3 * t * t * s) + P[3] * (t * t * t);
            bern = bern && near(bezierPoint(P, t), b, 1e-5f);
        }
        expect(bern, "de Casteljau = cong thuc Bernstein");
        const std::vector<Vec3> lv = deCasteljauLevels(P, 0.5f);
        expect(lv.size() == 6 && near(lv[0], {0.5f, 1, 0}) && near(lv[5], bezierPoint(P, 0.5f)), "de Casteljau: 3 + 2 + 1 diem trung gian");
        const float h = 1e-3f;
        const Vec3 numeric = (bezierPoint(P, 0.4f + h) - bezierPoint(P, 0.4f - h)) / (2 * h);
        expect(near(bezierTangent(P, 0.4f), numeric, 1e-2f), "dao ham Bezier khop dao ham so");
        expect(near(bezierTangent(P, 0), (P[1] - P[0]) * 3) && near(bezierTangent(P, 1), (P[3] - P[2]) * 3), "B'(0) = 3(P1 - P0), B'(1) = 3(P3 - P2)");

        // The tich co dau cua luoi kin: > 0 khi moi mat huong ra ngoai; bang the tich that khi luoi kin.
        auto volume = [](const Mesh& m) {
            double v = 0;
            for (const auto& t : m.triangles)
                v += dot(m.vertices[t[0]].position, cross(m.vertices[t[1]].position, m.vertices[t[2]].position)) / 6.0;
            return v;
        };
        auto relErr = [](double a, double b) { return std::fabs(a - b) / b; };
        std::string err;
        Material mat;
        const float r = 0.5f;
        const Mesh sphere = makeSphere(r, 32, 48, mat);
        expect(validate(sphere, err) && !sphere.triangles.empty(), "qua cau: luoi hop le");
        expect(relErr(volume(sphere), 4.0 / 3 * kPi * r * r * r) < 0.01, "qua cau: kin, dung chieu, the tich 4/3 pi r^3 (sai < 1 %)");
        bool onSphere = true, radialNormal = true;
        int poles = 0;
        for (const Vertex& v : sphere.vertices) {
            onSphere = onSphere && std::fabs(length(v.position) - r) < 1e-3f;
            radialNormal = radialNormal && dot(v.normal, normalize(v.position)) > 0.999f;
            if (std::fabs(std::fabs(v.position.y) - r) < 1e-6f) {
                ++poles;
                radialNormal = radialNormal && near(v.normal, {0, v.position.y > 0 ? 1.0f : -1.0f, 0});
            }
        }
        expect(onSphere, "qua cau: moi dinh cach tam dung r (Bezier xap xi duong tron)");
        expect(radialNormal, "qua cau: phap tuyen giai tich = huong ban kinh, cuc = (0, +-1, 0)");
        expect(poles == 2, "qua cau: moi cuc gom thanh 1 dinh");
        bool unit = true;
        for (const Vertex& v : sphere.vertices) unit = unit && std::fabs(length(v.normal) - 1) < 1e-4f;
        expect(unit, "mat tron xoay: phap tuyen do dai 1");

        const Mesh cyl = makeCylinder(0.3f, 1.0f, 64, mat);
        expect(validate(cyl, err) && relErr(volume(cyl), kPi * 0.09 * 1.0) < 0.01, "hinh tru: kin, the tich pi r^2 h");
        int capUp = 0, sideAtTop = 0;
        for (const Vertex& v : cyl.vertices)
            if (std::fabs(v.position.y - 1) < 1e-6f && length(Vec3{v.position.x, 0, v.position.z}) > 0.29f) {
                if (near(v.normal, {0, 1, 0})) ++capUp;
                else if (std::fabs(v.normal.y) < 1e-6f) ++sideAtTop;
            }
        expect(capUp == 65 && sideAtTop == 65, "hinh tru: mep nap tach 2 vong dinh (phap tuyen nap va than) -> canh sac");
        const Mesh cone = makeCone(0.4f, 0.9f, 64, mat);
        expect(validate(cone, err) && relErr(volume(cone), kPi * 0.16 * 0.9 / 3) < 0.01, "hinh non: kin, the tich pi r^2 h / 3");
        const Mesh torus = makeTorus(0.5f, 0.15f, 32, 48, mat);
        expect(validate(torus, err) && relErr(volume(torus), 2 * kPi * kPi * 0.5 * 0.15 * 0.15) < 0.01, "xuyen: kin, the tich 2 pi^2 R a^2");

        for (int i = 0; i < 5; ++i) {
            const Profile pr = exhibitProfile(i);
            bool c0 = true;
            for (std::size_t k = 1; k < pr.segments.size(); ++k) c0 = c0 && near(pr.segments[k - 1][3], pr.segments[k][0]);
            const Mesh m = revolve(pr, 8, 32, mat);
            expect(c0 && validate(m, err) && volume(m) > 0, "vat trung bay " + std::to_string(i) + ": profile lien tuc, luoi hop le, kin va dung chieu");
        }
    }

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

    {
        // Chieu sang Phong (DHMTPTB3-24).
        Material m;
        m.ka = {0.1f, 0.1f, 0.1f}; m.kd = {0.5f, 0.5f, 0.5f}; m.ks = {0.4f, 0.4f, 0.4f}; m.shininess = 8;
        const std::vector<Light> above = {Light{{0, 5, 0}, {1, 1, 1}}};
        const Vec3 P{0, 0, 0}, N{0, 1, 0};
        // Den va mat cung thang dung tren diem: N.L = 1, R = V -> I = ka + kd + ks.
        expect(near(shadePhong(P, N, {0, 3, 0}, above, m, false), {1.0f, 1.0f, 1.0f}), "Phong: den va mat thang dung -> ka + kd + ks");
        // Den o goc 60 do: N.L = 0,5; mat o phia doi dien phan xa -> R.V = 1.
        const Vec3 L60 = {std::sin(radians(60)) * 5, std::cos(radians(60)) * 5, 0};
        const Vec3 eyeMirror = {-L60.x, L60.y, 0};
        const Vec3 i60 = shadePhong(P, N, eyeMirror, {Light{L60, {1, 1, 1}}}, m, false);
        expect(std::fabs(i60.x - (0.1f + 0.5f * 0.5f + 0.4f)) < 1e-4f, "Phong: N.L = cos 60 = 0,5, mat dung huong phan xa");
        // Mat lech khoi huong phan xa -> vet sang giam theo mu n.
        const Vec3 iOff = shadePhong(P, N, {0, 3, 0}, {Light{L60, {1, 1, 1}}}, m, false);
        const float rv = std::cos(radians(60));  // goc giua R va V = 60 do
        expect(std::fabs(iOff.x - (0.1f + 0.25f + 0.4f * std::pow(rv, 8.0f))) < 1e-4f, "Phong: specular = ks * (R.V)^n");
        // Blinn: H nam giua L va V -> N.H = cos 30 do khi L lech 60 do, V thang dung.
        const Vec3 iBlinn = shadePhong(P, N, {0, 3, 0}, {Light{L60, {1, 1, 1}}}, m, true);
        expect(std::fabs(iBlinn.x - (0.1f + 0.25f + 0.4f * std::pow(std::cos(radians(30)), 8.0f))) < 1e-4f, "Blinn-Phong: dung N.H");
        // Den sau mat: chi con ka.
        expect(near(shadePhong(P, N, {0, 3, 0}, {Light{{0, -5, 0}, {1, 1, 1}}}, m, false), m.ka), "Phong: den sau mat -> chi con moi truong");
        // Hai den cong lai, mau den nhan tung kenh.
        const Vec3 two = shadePhong(P, N, {0, 3, 0}, {Light{{0, 5, 0}, {1, 0, 0}}, Light{{0, 5, 0}, {0, 0, 1}}}, m, false);
        expect(near(two, {1.0f, 0.1f, 1.0f}), "Phong: nhieu den cong lai, mau den nhan theo kenh");

        // Back-face (DHMTPTB3-37): nguoc chieu kim dong ho trong the gioi -> tren man hinh (y xuong) la cung chieu.
        ScreenVertex front[3], back[3];
        front[0].x = 0; front[0].y = 10; front[1].x = 10; front[1].y = 10; front[2].x = 0; front[2].y = 0;
        back[0] = front[0]; back[1] = front[2]; back[2] = front[1];
        expect(!isBackFace(front) && isBackFace(back), "Back-face: dau dien tich tren man hinh");
        // Hop nhin tu ngoai: dung mot nua so mat bi bo (6 trong 12), phong nhin tu trong: khong bo mat nao cua tuong.
        Framebuffer fb;
        fb.resize(160, 90);
        SceneObject box;
        box.mesh = makeBox({0.5f, 0.5f, 0.5f}, Material{});
        box.model = translate({0.3f, 0.2f, 0}) * rotateY(radians(30));
        Camera cam;
        cam.eye = {0, 0.8f, 3};
        cam.pitch = radians(-12);
        RenderSettings rs;
        FrameStats st;
        renderScene(fb, {box}, cam, {Light{}}, rs, st);
        expect(st.culledBackFace == 6, "Back-face: hop nhin tu ngoai bo dung 6/12 tam giac");

        // Z-buffer (DHMTPTB3-32): hai tam giac chong nhau, ve XA SAU cung -> van bi che nho Z-buffer.
        auto quad = [](float z, Vec3 kd) {
            SceneObject o;
            o.mesh.vertices = {{{-1, -1, z}, {0, 0, 1}}, {{1, -1, z}, {0, 0, 1}}, {{1, 1, z}, {0, 0, 1}}, {{-1, 1, z}, {0, 0, 1}}};
            o.mesh.triangles = {{0, 1, 2}, {0, 2, 3}};
            o.mesh.material.kd = kd; o.mesh.material.ka = {0, 0, 0}; o.mesh.material.ks = {0, 0, 0};
            return o;
        };
        const std::vector<SceneObject> pair = {quad(-1, {1, 0, 0}), quad(-3, {0, 0, 1})};  // do gan, xanh xa
        Camera c0;
        c0.eye = {0, 0, 2};
        auto centerColor = [&](const RenderSettings& r) {
            fb.clear({0, 0, 0});
            renderScene(fb, pair, c0, {Light{{0, 0, 5}, {1, 1, 1}}}, r, st);
            const std::size_t i = fb.index(fb.width / 2, fb.height / 2) * 4;
            return Vec3{float(fb.color[i]), float(fb.color[i + 1]), float(fb.color[i + 2])};
        };
        RenderSettings zOn;
        const Vec3 cz = centerColor(zOn);
        expect(cz.x > 100 && cz.z < 10, "Z-buffer: vat gan (do) che vat xa (xanh) du ve truoc");
        RenderSettings zOff;
        zOff.zBuffer = false;
        const Vec3 cn = centerColor(zOff);
        expect(cn.z > 100 && cn.x < 10, "tat Z-buffer: vat ve sau (xanh, xa) de len vat gan -> sai");
        // Ve theo thu tu nguoc lai van dung khi bat Z-buffer.
        const float zCenter = fb.depthAt(fb.width / 2, fb.height / 2);
        expect(std::isinf(zCenter), "tat Z-buffer: khong ghi mang do sau");
        fb.clear({0, 0, 0});
        renderScene(fb, pair, c0, {Light{}}, zOn, st);
        expect(std::fabs(fb.depthAt(fb.width / 2, fb.height / 2) - 3) < 1e-3f, "Z-buffer: do sau tai tam = khoang cach toi vat gan (3 m)");

        // Anh do sau (phim V): gan sang hon xa, nen den.
        RenderSettings dv;
        dv.showDepth = true;
        const std::vector<SceneObject> sideBySide = {quad(-1, {1, 0, 0})};
        fb.clear({0, 0, 0});
        renderScene(fb, sideBySide, c0, {Light{}}, dv, st);
        const std::size_t ci = fb.index(fb.width / 2, fb.height / 2) * 4, corner = fb.index(0, 0) * 4;
        expect(fb.color[ci] > 200 && fb.color[ci] == fb.color[ci + 1] && fb.color[corner] == 0, "anh do sau: vat sang xam, nen den");

        // Gouraud vs Phong: den chieu sat tam mot mat phang lon -> Phong thay vet sang giua mat, Gouraud (chi tinh o dinh) thi khong.
        SceneObject plane = quad(-2, {0.2f, 0.2f, 0.2f});
        plane.mesh.material.ks = {1, 1, 1}; plane.mesh.material.shininess = 64;
        const std::vector<Light> spot = {Light{{0, 0, 0}, {1, 1, 1}}};
        Camera c1;
        c1.eye = {0, 0, 0};
        auto center = [&](ShadingMode mode) {
            RenderSettings r;
            r.shading = mode;
            fb.clear({0, 0, 0});
            renderScene(fb, {plane}, c1, spot, r, st);
            return int(fb.color[fb.index(fb.width / 2, fb.height / 2) * 4]);
        };
        const int gouraudC = center(ShadingMode::Gouraud), phongC = center(ShadingMode::Phong);
        expect(phongC > 230 && gouraudC < 120, "Phong bat duoc vet sang giua mat, Gouraud bo lo");
        std::printf("  vet sang giua mat: Gouraud %d, Phong %d (0-255)\n", gouraudC, phongC);
    }

    {
        // Bresenham (DHMTPTB3-35): du 8 huong, so pixel = max(|dx|, |dy|) + 1, 2 dau mut duoc to,
        // cac pixel lien tiep ke nhau (8-lien thong), lech khoi duong thang that < 0,5 pixel.
        Framebuffer fb;
        fb.resize(41, 41);
        auto lit = [&](int x, int y) { return fb.color[fb.index(x, y) * 4] > 0; };
        bool allOk = true;
        const int ends[8][2] = {{20, 7}, {7, 20}, {-7, 20}, {-20, 7}, {-20, -7}, {-7, -20}, {7, -20}, {20, -7}};
        for (const auto& e : ends) {
            fb.clear({0, 0, 0});
            const int x0 = 20, y0 = 20, x1 = 20 + e[0] / 1, y1 = 20 + e[1] / 1;
            drawLineBresenham(fb, x0, y0, x1, y1, {1, 1, 1});
            int count = 0;
            float worst = 0;
            for (int y = 0; y < 41; ++y)
                for (int x = 0; x < 41; ++x)
                    if (lit(x, y)) {
                        ++count;
                        // khoang cach tu tam pixel toi duong thang that
                        const float dx = float(x1 - x0), dy = float(y1 - y0);
                        worst = std::max(worst, std::fabs(dy * (x - x0) - dx * (y - y0)) / std::sqrt(dx * dx + dy * dy));
                    }
            allOk = allOk && count == std::max(std::abs(e[0]), std::abs(e[1])) + 1 && lit(x0, y0) && lit(x1, y1) && worst <= 0.5f + 1e-4f;
        }
        expect(allOk, "Bresenham: 8 huong, dung so pixel, qua 2 dau mut, lech < 0,5 pixel");
        fb.clear({0, 0, 0});
        drawLineBresenham(fb, 3, 5, 3, 5, {1, 1, 1});
        expect(lit(3, 5), "Bresenham: doan suy bien thanh 1 diem");

        // Liang–Barsky: doan trong khung giu nguyen, doan cat khung bi cat dung bien, doan ngoai bi bo.
        float ax = 5, ay = 5, bx = 30, by = 20;
        expect(clipLineToRect(ax, ay, bx, by, 41, 41) && ax == 5 && bx == 30, "Liang-Barsky: doan nam trong khung giu nguyen");
        ax = -1e6f; ay = 20; bx = 1e6f; by = 20;
        expect(clipLineToRect(ax, ay, bx, by, 41, 41) && std::fabs(ax) < 1e-2f && std::fabs(bx - 40) < 1e-2f && ay == 20,
               "Liang-Barsky: toa do hang trieu pixel bi cat ve [0, 40]");
        ax = -5; ay = -5; bx = -1; by = 30;
        expect(!clipLineToRect(ax, ay, bx, by, 41, 41), "Liang-Barsky: doan nam ngoai khung -> bo");
    }
    {
        // Painter (DHMTPTB3-38).
        auto quad = [](float z, Vec3 kd) {
            SceneObject o;
            o.mesh.vertices = {{{-1, -1, z}, {0, 0, 1}}, {{1, -1, z}, {0, 0, 1}}, {{1, 1, z}, {0, 0, 1}}, {{-1, 1, z}, {0, 0, 1}}};
            o.mesh.triangles = {{0, 1, 2}, {0, 2, 3}};
            o.mesh.material.kd = kd; o.mesh.material.ka = {0, 0, 0}; o.mesh.material.ks = {0, 0, 0};
            return o;
        };
        Framebuffer fb;
        fb.resize(160, 90);
        Camera c0;
        c0.eye = {0, 0, 2};
        FrameStats st;
        RenderSettings painter;
        painter.painter = true;
        // Danh sach: vat gan (do) truoc, vat xa (xanh) sau. Painter phai tu sap xep lai.
        fb.clear({0, 0, 0});
        renderScene(fb, {quad(-1, {1, 0, 0}), quad(-3, {0, 0, 1})}, c0, {Light{{0, 0, 5}, {1, 1, 1}}}, painter, st);
        const std::size_t ci = fb.index(80, 45) * 4;
        expect(fb.color[ci] > 100 && fb.color[ci + 2] < 10 && std::isinf(fb.depthAt(80, 45)),
               "Painter: sap xep xa -> gan, vat gan che vat xa ma khong dung Z-buffer");

        // Canh qua cau xuyen tam phang: Painter ve khac han Z-buffer (sai), Z-buffer la anh dung.
        fb.resize(320, 180);
        const std::vector<SceneObject> demo = buildPainterDemo();
        expect(demo.size() == 3, "canh Painter: qua cau, tam phang, san");
        Camera c;
        c.mode = Camera::Mode::Orbit; c.target = kPainterDemoCenter; c.orbitRadius = 2.6f; c.orbitTheta = 0.9f; c.orbitPhi = 0.3f;
        const std::vector<Light> L = {Light{{1.5f, 3, 2.5f}, {1, 1, 1}}};
        RenderSettings z;
        fb.clear({0, 0, 0});
        renderScene(fb, demo, c, L, z, st);
        const std::vector<std::uint8_t> correct = fb.color;
        fb.clear({0, 0, 0});
        renderScene(fb, demo, c, L, painter, st);
        int diff = 0;
        for (std::size_t i = 0; i < correct.size(); i += 4)
            if (std::abs(int(correct[i]) - int(fb.color[i])) + std::abs(int(correct[i + 2]) - int(fb.color[i + 2])) > 60) ++diff;
        expect(diff > 500, "canh Painter: hai vat cat nhau -> Painter ve sai nhieu pixel so voi Z-buffer");
        std::printf("  canh Painter: %d pixel khac Z-buffer\n", diff);
    }

    {
        // Bong do phang (DHMTPTB3-44): P' = S P nam tren san va thang hang voi den L va diem P.
        const Vec3 L{1, 4, -2};
        const Mat4 S = planarShadowMatrix(L);
        bool onRay = true;
        for (Vec3 P : {Vec3{0, 1, 0}, Vec3{2, 0.5f, 1}, Vec3{-1, 3, 3}}) {
            const Vec4 h = S * Vec4{P.x, P.y, P.z, 1};
            const Vec3 Q{h.x / h.w, h.y / h.w, h.z / h.w};
            onRay = onRay && std::fabs(Q.y) < 1e-5f && length(cross(P - L, Q - L)) < 1e-4f && dot(P - L, Q - L) > 0;
        }
        expect(onRay, "bong: S chieu P len y = 0 theo tia tu den qua P");
        const Vec4 foot = S * Vec4{L.x, 0, L.z, 1};
        expect(near({foot.x / foot.w, 0, foot.z / foot.w}, {L.x, 0, L.z}), "bong: diem tren san giu nguyen (S P = P)");

        // Hop 1 x 1 x 1 treo cao 1 m, den thang tren: bong la hinh vuong to hon (phong to Ly / (Ly - y)).
        std::vector<SceneObject> objs(1);
        objs[0].mesh = makeBox({0.5f, 0.5f, 0.5f}, Material{});
        objs[0].model = translate({0, 1.5f, 0});
        std::vector<SceneObject> sh;
        Material sm;
        buildPlanarShadows(objs, 0, {Light{{0, 4, 0}, {1, 1, 1}}}, sm, 6, 6, sh);
        bool flat = sh.size() == 1;
        float maxX = 0;
        double upArea = 0;
        for (const Vertex& v : sh[0].mesh.vertices) { flat = flat && std::fabs(v.position.y - 0.003f) < 1e-6f; maxX = std::max(maxX, v.position.x); }
        for (const auto& t : sh[0].mesh.triangles) {
            const Vec3 n = faceNormal(sh[0].mesh, t);
            if (n.y > 0) upArea += n.y / 2;
        }
        // Mat tren (y = 2) xa san nhat nen phong to nhieu nhat: nua canh 0,5 * 4 / (4 - 2) = 1 -> bong 2 x 2.
        // Den thang tren: chi mat tren huong ve den -> chieu xuong van huong len; 4 mat ben va mat day bi lat.
        expect(flat && std::fabs(maxX - 1.0f) < 1e-4f, "bong: nam sat san, canh xa nhat theo ti le Ly / (Ly - y)");
        expect(std::fabs(upArea - 4.0) < 1e-3, "bong: mat huong len (khong bi Back-face bo) phu dung 1 lop = dien tich bong");

        // Den nam thap hon vat: dinh khong co bong -> bo tam giac.
        buildPlanarShadows(objs, 0, {Light{{0, 1.2f, 0}, {1, 1, 1}}}, sm, 6, 6, sh);
        expect(sh[0].mesh.triangles.size() < objs[0].mesh.triangles.size(), "bong: dinh cao hon den bi bo, khong chia cho w <= 0");

        // Qua cau den (DHMTPTB3-43): mau = mau den (da chuan hoa), khong phu thuoc anh sang.
        const std::vector<Light> lights = {Light{{0, 0, -3}, {0.5f, 0.25f, 0.1f}}, Light{{1, 0, -3}, {0.2f, 0.2f, 0.4f}}};
        const std::vector<SceneObject> marks = buildLightMarkers(lights, 0.3f);
        expect(marks.size() == 2 && near(marks[0].mesh.material.ka, {1, 0.5f, 0.2f}) && near(transformPoint(marks[1].model, {0, 0, 0}), {1, 0, -3}),
               "den: moi den mot qua cau, mau chuan hoa, dat dung vi tri");
        Framebuffer fb;
        fb.resize(160, 90);
        Camera c;
        c.eye = {0, 0, 0};
        RenderSettings rs;
        rs.shading = ShadingMode::Phong;
        FrameStats st;
        fb.clear({0, 0, 0});
        renderScene(fb, {marks[0]}, c, lights, rs, st);
        const std::size_t ci = fb.index(80, 45) * 4;
        expect(fb.color[ci] == 255 && fb.color[ci + 1] == 128 && fb.color[ci + 2] == 51, "den: qua cau phat sang dung mau den o moi che do to");
        const Ray ray = rayFromPixel(c, 80, 45, 160, 90, 160.0f / 90);
        expect(pick(marks, ray).index == 0, "den: click trung qua cau den chon dung den de keo");
    }

    std::printf("%d dung, %d sai\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
