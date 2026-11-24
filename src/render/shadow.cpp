// PHAN C — BONG DO PHANG VA DEN (Lam, Sprint 5: DHMTPTB3-43, DHMTPTB3-44).
#include "shadow.hpp"

#include <algorithm>
#include <cmath>

#include "modeling/modeling.hpp"
#include "render/render.hpp"

Mat4 planarShadowMatrix(Vec3 L) {
    Mat4 S;
    S.m[0][0] = L.y; S.m[0][1] = -L.x;
    S.m[2][1] = -L.z; S.m[2][2] = L.y;
    S.m[3][1] = -1;  S.m[3][3] = L.y;
    return S;
}

void appendPlanarShadows(const std::vector<const SceneObject*>& casters, const Light& light,
                         const Material& shadowMaterial, float roomHalfX, float roomHalfZ,
                         std::vector<SceneObject>& out, float lift) {
    const Mat4 S = planarShadowMatrix(light.position);
    for (const SceneObject* cp : casters) {
        const SceneObject& c = *cp;
        if (!c.visible || c.mesh.triangles.empty()) continue;
        SceneObject shadow;
        shadow.mesh.name = "bong " + c.mesh.name;
        shadow.mesh.material = shadowMaterial;
        shadow.mesh.vertices.resize(c.mesh.vertices.size());
        std::vector<bool> ok(c.mesh.vertices.size());
        for (std::size_t i = 0; i < c.mesh.vertices.size(); ++i) {
            const Vec3 p = transformPoint(c.model, c.mesh.vertices[i].position);
            const Vec4 s = S * Vec4{p.x, p.y, p.z, 1};
            ok[i] = s.w > 1e-4f;  // diem thap hon den
            Vertex& v = shadow.mesh.vertices[i];
            v.position = ok[i] ? Vec3{s.x / s.w, lift, s.z / s.w} : Vec3{p.x, lift, p.z};
            v.normal = {0, 1, 0};
            // Cung cach dat (u, v) nhu mat san cua makeBox (phong 2*halfX x 2*halfZ, tam o goc).
            v.u = (v.position.x / roomHalfX + 1) * 0.5f * roomHalfX;
            v.v = (v.position.z / roomHalfZ + 1) * 0.5f * roomHalfZ;
        }
        for (const auto& t : c.mesh.triangles)
            if (ok[t[0]] && ok[t[1]] && ok[t[2]]) shadow.mesh.triangles.push_back(t);
        // Mat huong ve den chieu xuong van giu chieu nguoc kim dong ho nhin tu tren;
        // mat quay lung voi den bi lat -> Back-face bo di, khong ve 2 lop.
        out.push_back(std::move(shadow));
    }
}

void buildPlanarShadows(const std::vector<SceneObject>& casters, std::size_t first, const std::vector<Light>& lights,
                        const Material& shadowMaterial, float roomHalfX, float roomHalfZ,
                        std::vector<SceneObject>& out, float lift) {
    out.clear();
    std::vector<const SceneObject*> ptrs;
    for (std::size_t k = first; k < casters.size(); ++k) ptrs.push_back(&casters[k]);
    for (const Light& light : lights) appendPlanarShadows(ptrs, light, shadowMaterial, roomHalfX, roomHalfZ, out, lift);
}

std::vector<SceneObject> buildLightMarkers(const std::vector<Light>& lights, float radius) {
    std::vector<SceneObject> markers;
    for (std::size_t i = 0; i < lights.size(); ++i) {
        const Light& l = lights[i];
        const float peak = std::max({l.color.x, l.color.y, l.color.z, 1e-3f});
        Material m;
        m.name = "den";
        m.ka = l.color / peak;  // I = ka * Ia: sang deu, khong phu thuoc den khac
        m.kd = {0, 0, 0};
        m.ks = {0, 0, 0};
        SceneObject o;
        o.mesh = makeSphere(radius, 10, 16, m);
        o.mesh.name = "den " + std::to_string(i + 1);
        o.model = translate(l.position);
        markers.push_back(std::move(o));
    }
    return markers;
}

std::vector<SceneObject> buildPendantLamp(const Light& light, float ceiling) {
    const float peak = std::max({light.color.x, light.color.y, light.color.z, 1e-3f});
    const Vec3 glow = light.color / peak;

    Material bulbMat;  // phat sang: chi co ka (I = ka * Ia), khong phu thuoc den nao
    bulbMat.name = "bong den";
    bulbMat.ka = Vec3{1, 1, 1} * 0.35f + glow * 0.65f;
    bulbMat.kd = bulbMat.ks = {0, 0, 0};
    Material inner = bulbMat;  // long chao: duoc bong den chieu sang
    inner.name = "long chao den";
    inner.ka = glow * 0.75f;
    Material outer;
    outer.name = "chao den dong";
    outer.kd = {0.28f, 0.22f, 0.15f};
    outer.ka = outer.kd * 0.3f;
    outer.ks = {0.7f, 0.6f, 0.45f};
    outer.shininess = 60;

    auto P = [](float x, float y) { return Vec3{x, y, 0}; };
    // Mat ngoai: tu mep chao di len (phap tuyen huong ra ngoai - len).
    Profile out;
    out.segments = {bezierLine(P(0.20f, -0.06f), P(0.05f, 0.14f)), bezierLine(P(0.05f, 0.14f), P(0, 0.14f))};
    // Mat trong: tu truc ra roi di xuong mep (phap tuyen huong vao trong - xuong) -> nhin tu duoi thay long chao.
    Profile in;
    in.segments = {bezierLine(P(0, 0.125f), P(0.045f, 0.125f)), bezierLine(P(0.045f, 0.125f), P(0.195f, -0.06f))};

    Mesh shade = revolve(out, 2, 28, outer);
    const float rod = std::max(0.05f, ceiling - light.position.y - 0.14f);
    const Mesh wire = makeCylinder(0.008f, rod, 8, outer);
    const Mesh canopy = makeCylinder(0.06f, 0.03f, 16, outer);
    const int base = static_cast<int>(shade.vertices.size());
    for (Vertex v : wire.vertices) { v.position.y += 0.14f; shade.vertices.push_back(v); }
    for (const auto& t : wire.triangles) shade.triangles.push_back({base + t[0], base + t[1], base + t[2]});
    const int base2 = static_cast<int>(shade.vertices.size());
    for (Vertex v : canopy.vertices) { v.position.y += 0.14f + rod - 0.03f; shade.vertices.push_back(v); }
    for (const auto& t : canopy.triangles) shade.triangles.push_back({base2 + t[0], base2 + t[1], base2 + t[2]});
    shade.name = "den tha tran";

    std::vector<SceneObject> parts(3);
    parts[0].mesh = makeSphere(0.06f, 8, 14, bulbMat);
    parts[0].mesh.name = "bong den";
    parts[1].mesh = std::move(shade);
    parts[2].mesh = revolve(in, 2, 28, inner);
    parts[2].mesh.name = "long chao den";
    for (SceneObject& p : parts) p.model = translate(light.position);
    return parts;
}

void drawNormals(Framebuffer& fb, const std::vector<SceneObject>& objects, std::size_t first, const Camera& camera,
                 float len, Vec3 color) {
    std::vector<const SceneObject*> ptrs;
    for (std::size_t k = first; k < objects.size(); ++k) ptrs.push_back(&objects[k]);
    drawNormals(fb, ptrs, camera, len, color);
}

void drawNormals(Framebuffer& fb, const std::vector<const SceneObject*>& objects, const Camera& camera,
                 float len, Vec3 color) {
    const Mat4 view = camera.viewMatrix();
    const Mat4 proj = camera.projectionMatrix(static_cast<float>(fb.width) / fb.height);
    const Vec3 eye = camera.eyePosition();
    const float zn = -camera.nearPlane;
    auto toScreen = [&](Vec3 v, float& x, float& y) {
        const Vec4 c = proj * Vec4{v.x, v.y, v.z, 1};
        x = (c.x / c.w * 0.5f + 0.5f) * fb.width;
        y = (0.5f - c.y / c.w * 0.5f) * fb.height;
    };
    for (const SceneObject* op : objects) {
        const SceneObject& o = *op;
        if (!o.visible) continue;
        for (const Vertex& vtx : o.mesh.vertices) {
            const Vec3 p = transformPoint(o.model, vtx.position);
            const Vec3 n = normalize(transformDir(o.model, vtx.normal));
            if (camera.projection == Projection::Perspective ? dot(n, eye - p) <= 0 : dot(n, -camera.forward()) <= 0) continue;
            const Vec3 a = transformPoint(view, p), b = transformPoint(view, p + n * len);
            if (a.z > zn || b.z > zn) continue;  // gach cham mat phang gan: bo (ngan, khong can xen)
            float ax, ay, bx, by;
            toScreen(a, ax, ay);
            toScreen(b, bx, by);
            // Goc gach bi vat khac che (Z-buffer da co do sau gan hon) -> khong ve, tranh thay
            // phap tuyen "xuyen" qua buc hay tuong. Do sau trong Z-buffer = -z he quan sat.
            const int sx = static_cast<int>(ax), sy = static_cast<int>(ay);
            if (fb.inside(sx, sy) && fb.depthAt(sx, sy) < -a.z - 0.02f) continue;
            if (clipLineToRect(ax, ay, bx, by, float(fb.width), float(fb.height)))
                drawLineBresenham(fb, int(std::lround(ax)), int(std::lround(ay)), int(std::lround(bx)), int(std::lround(by)), color);
        }
    }
}
