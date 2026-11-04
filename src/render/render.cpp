// PHAN C — HIEN THI (Lam). Ham renderScene da noi san cac buoc; cac thuat toan con TODO.
#include "render.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

Vec3 shadePhong(Vec3 point, Vec3 normal, Vec3 eye, const std::vector<Light>& lights,
                const Material& material, bool blinn) {
    // TODO(Lam, Sprint 2): I = ka*Ia + sum[ kd*max(N.L,0) + ks*max(R.V,0)^n ] * IL
    // (blinn: thay R.V bang N.H, H = normalize(L + V)).
    (void)point; (void)normal; (void)eye; (void)lights; (void)blinn;
    return material.kd;
}

bool isBackFace(const ScreenVertex tri[3]) {
    // TODO(Lam, Sprint 4): dau cua dien tich tam giac tren man hinh (hoac N.(P - E) > 0 trong he quan sat).
    (void)tri;
    return false;
}

void fillTriangle(Framebuffer& fb, const ScreenVertex tri[3], const ShadeContext& ctx, const RenderSettings& s) {
    // TODO(Lam, Sprint 1): to dong quet — sap xep 3 dinh theo y, di theo 2 canh trai/phai,
    // moi dong y to tu xTrai den xPhai. Sprint 2-3: noi suy color/normal; Sprint 3: Z-buffer.
    (void)fb; (void)tri; (void)ctx; (void)s;
}

void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Vec3 color) {
    // TODO(Khang, Sprint 4): Bresenham 8 truong hop huong (code Lab 01).
    (void)fb; (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
}

namespace {

ScreenVertex project(const ViewVertex& v, const Mat4& proj, int w, int h) {
    const Vec4 c = proj * Vec4{v.view.x, v.view.y, v.view.z, 1};
    const float iw = (c.w != 0) ? 1.0f / c.w : 1.0f;
    ScreenVertex s;
    s.x = (c.x * iw * 0.5f + 0.5f) * w;          // NDC [-1,1] -> pixel
    s.y = (0.5f - c.y * iw * 0.5f) * h;          // y man hinh huong xuong
    s.z = -v.view.z;                              // khoang cach theo truc nhin
    s.world = v.world; s.normal = v.normal; s.u = v.u; s.v = v.v;
    return s;
}

double msSince(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

}  // namespace

void renderScene(Framebuffer& fb, const std::vector<SceneObject>& objects, const Camera& camera,
                 const std::vector<Light>& lights, const RenderSettings& settings, FrameStats& stats) {
    stats = FrameStats{};
    const Mat4 view = camera.viewMatrix();
    const Mat4 proj = camera.projectionMatrix(static_cast<float>(fb.width) / fb.height);
    const Vec3 eye = camera.eyePosition();

    struct Pending { ScreenVertex v[3]; const Material* mat; float depth; };
    std::vector<Pending> queue;

    auto t0 = std::chrono::steady_clock::now();
    for (const SceneObject& obj : objects) {
        if (!obj.visible) continue;
        const Mat4 mv = view * obj.model;
        std::vector<ViewVertex> vv(obj.mesh.vertices.size());
        for (std::size_t i = 0; i < vv.size(); ++i) {
            const Vertex& src = obj.mesh.vertices[i];
            vv[i].view = transformPoint(mv, src.position);
            vv[i].world = transformPoint(obj.model, src.position);
            vv[i].normal = normalize(transformDir(obj.model, src.normal));  // dung khi model khong co ti le lech
            vv[i].u = src.u; vv[i].v = src.v;
        }
        for (const auto& t : obj.mesh.triangles) {
            ++stats.trianglesIn;
            const ViewVertex tri[3] = {vv[t[0]], vv[t[1]], vv[t[2]]};
            const std::vector<ViewVertex> poly = clipNear(tri, camera.nearPlane);
            if (poly.size() < 3) { ++stats.clipped; continue; }
            for (std::size_t k = 1; k + 1 < poly.size(); ++k) {           // chia da giac thanh quat tam giac
                Pending p;
                p.v[0] = project(poly[0], proj, fb.width, fb.height);
                p.v[1] = project(poly[k], proj, fb.width, fb.height);
                p.v[2] = project(poly[k + 1], proj, fb.width, fb.height);
                if (settings.backFace && isBackFace(p.v)) { ++stats.culledBackFace; continue; }
                p.mat = &obj.mesh.material;
                p.depth = (p.v[0].z + p.v[1].z + p.v[2].z) / 3.0f;
                queue.push_back(p);
            }
        }
    }
    stats.msTransform = msSince(t0);

    auto t1 = std::chrono::steady_clock::now();
    if (settings.painter) {
        // TODO(Lam, Sprint 4): Painter — sap xep queue tu xa den gan roi ve de len nhau
        // (thay std::sort san co bang thuat toan sap xep minh tu cai neu thay yeu cau).
    }
    ShadeContext ctx;
    ctx.lights = &lights; ctx.eye = eye;
    for (Pending& p : queue) {
        ctx.material = p.mat;
        if (settings.shading == ShadingMode::Flat) {
            const Vec3 c = (p.v[0].world + p.v[1].world + p.v[2].world) / 3.0f;
            const Vec3 n = normalize(cross(p.v[1].world - p.v[0].world, p.v[2].world - p.v[0].world));
            p.v[0].color = shadePhong(c, n, eye, lights, *p.mat, settings.blinnPhong);
        } else if (settings.shading == ShadingMode::Gouraud) {
            for (ScreenVertex& v : p.v) v.color = shadePhong(v.world, v.normal, eye, lights, *p.mat, settings.blinnPhong);
        }
        if (settings.shading == ShadingMode::Wireframe) {
            const Vec3 line{0.9f, 0.9f, 0.9f};
            for (int e = 0; e < 3; ++e) {
                const ScreenVertex& a = p.v[e];
                const ScreenVertex& b = p.v[(e + 1) % 3];
                drawLineBresenham(fb, static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x), static_cast<int>(b.y), line);
            }
        } else {
            fillTriangle(fb, p.v, ctx, settings);
        }
        ++stats.trianglesDrawn;
    }
    stats.msRaster = msSince(t1);
}
