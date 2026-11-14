// PHAN C — BONG DO PHANG VA DEN (Lam, Sprint 5: DHMTPTB3-43, DHMTPTB3-44).
#include "shadow.hpp"

#include <algorithm>

#include "modeling/modeling.hpp"

Mat4 planarShadowMatrix(Vec3 L) {
    Mat4 S;
    S.m[0][0] = L.y; S.m[0][1] = -L.x;
    S.m[2][1] = -L.z; S.m[2][2] = L.y;
    S.m[3][1] = -1;  S.m[3][3] = L.y;
    return S;
}

void buildPlanarShadows(const std::vector<SceneObject>& casters, std::size_t first, const std::vector<Light>& lights,
                        const Material& shadowMaterial, float roomHalfX, float roomHalfZ,
                        std::vector<SceneObject>& out, float lift) {
    out.clear();
    for (const Light& light : lights) {
        const Mat4 S = planarShadowMatrix(light.position);
        for (std::size_t k = first; k < casters.size(); ++k) {
            const SceneObject& c = casters[k];
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
