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

    // --- C · Lam: SAU KHI CAI to dong quet ---
    // dem so pixel duoc to cua mot tam giac vuong 10x10 ~ 50.

    std::printf("%d dung, %d sai\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
