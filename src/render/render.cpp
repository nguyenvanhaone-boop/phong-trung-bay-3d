// PHAN C — HIEN THI (Lam). Chieu sang Phong, Flat/Gouraud/Phong, Back-face, Z-buffer, Painter; khung day Bresenham.
#include "render.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <thread>

namespace {

// Cuong do anh sang moi truong Ia (slide CG.6). Nhan voi ka cua vat lieu.
const Vec3 kAmbient{1, 1, 1};

// Phan loi cua cong thuc Phong, tach rieng he so de texture thay duoc kd, ka tung pixel
// (khong phai sao chep ca Material, vi Material co string va shared_ptr).
Vec3 phongTerms(Vec3 point, Vec3 normal, Vec3 eye, const std::vector<Light>& lights,
                Vec3 ka, Vec3 kd, Vec3 ks, float shininess, bool blinn) {
    const Vec3 N = normalize(normal);
    const Vec3 V = normalize(eye - point);              // tu diem toi mat
    Vec3 I = mul(ka, kAmbient);
    for (const Light& light : lights) {
        const Vec3 L = normalize(light.position - point);  // tu diem toi den
        const float NL = dot(N, L);
        if (NL <= 0) continue;                           // den nam sau mat: khong khuech tan, khong phan xa
        float spec;
        if (blinn) {
            const Vec3 H = normalize(L + V);             // vecto nua goc (half-vector)
            spec = std::max(dot(N, H), 0.0f);
        } else {
            const Vec3 R = N * (2 * NL) - L;             // L phan xa qua N: R = 2(N.L)N - L
            spec = std::max(dot(R, V), 0.0f);
        }
        const Vec3 term = kd * NL + ks * (spec > 0 ? std::pow(spec, shininess) : 0.0f);
        I = I + mul(term, light.color);
    }
    return I;
}

}  // namespace

// Mo hinh chieu sang Phong (slide CG.6):
//   I = ka*Ia + sum_den IL * [ kd * max(N.L, 0) + ks * max(R.V, 0)^n ]
//   N: phap tuyen, L: huong toi den, V: huong toi mat, R = 2(N.L)N - L: L phan xa qua N.
// Blinn-Phong thay R.V bang N.H voi H = normalize(L + V): re hon (khong can tinh R),
// vet sang rong hon mot chut voi cung so mu n.
Vec3 shadePhong(Vec3 point, Vec3 normal, Vec3 eye, const std::vector<Light>& lights,
                const Material& material, bool blinn) {
    return phongTerms(point, normal, eye, lights, material.ka, material.kd, material.ks, material.shininess, blinn);
}

// Back-face (slide CG.6): mat truoc co dinh xep NGUOC chieu kim dong ho khi nhin tu ngoai (mesh.hpp).
// Tren man hinh truc y huong XUONG nen chieu bi lat: mat truoc co dien tich co dau < 0.
//   dien tich co dau = (x1 - x0)(y2 - y0) - (y1 - y0)(x2 - x0)
// >= 0 -> quay lung (hoac suy bien thanh doan thang) -> bo.
// Lam tren man hinh (sau khi chieu) nen dung cho ca phoi canh lan song song.
bool isBackFace(const ScreenVertex tri[3]) {
    const double area = (double(tri[1].x) - tri[0].x) * (double(tri[2].y) - tri[0].y)
                      - (double(tri[1].y) - tri[0].y) * (double(tri[2].x) - tri[0].x);
    return area >= 0;
}

void interpolationWeights(const ScreenVertex tri[3], float px, float py, bool perspective, float w[3]) {
    // Toa do trong tam tren man hinh: ti so dien tich tam giac con / dien tich ca tam giac.
    const double x0 = tri[0].x, y0 = tri[0].y;
    const double e1x = double(tri[1].x) - x0, e1y = double(tri[1].y) - y0;
    const double e2x = double(tri[2].x) - x0, e2y = double(tri[2].y) - y0;
    const double area = e1x * e2y - e1y * e2x;
    if (area == 0.0) { w[0] = 1; w[1] = w[2] = 0; return; }
    const double dx = px - x0, dy = py - y0;
    const double b1 = (dx * e2y - dy * e2x) / area;
    const double b2 = (e1x * dy - e1y * dx) / area;
    double b[3] = {1 - b1 - b2, b1, b2};
    if (perspective) {
        double sum = 0;
        for (int i = 0; i < 3; ++i) { b[i] /= std::max(double(tri[i].z), 1e-6); sum += b[i]; }
        for (double& bi : b) bi /= sum;
    }
    for (int i = 0; i < 3; ++i) w[i] = static_cast<float>(b[i]);
}

void fillTriangle(Framebuffer& fb, const ScreenVertex tri[3], const ShadeContext& ctx, const RenderSettings& s,
                  int rowBegin, int rowEnd) {
    const Texture* texture = (s.texture && ctx.material && ctx.material->texture) ? ctx.material->texture.get() : nullptr;
    const bool perspective = s.perspectiveCorrect && ctx.perspective;
    if (fb.width <= 0 || fb.height <= 0 || s.shading == ShadingMode::Wireframe) return;
    for (int vertex = 0; vertex < 3; ++vertex)
        if (!std::isfinite(tri[vertex].x) || !std::isfinite(tri[vertex].y)) return;

    // ---- To mot pixel (Sprint 2-5) ----
    // Moi pixel tinh toa do trong tam (b0, b1, b2) tren man hinh MOT lan, roi dung chung cho:
    //   - Gouraud: noi suy mau tuyen tinh tren man hinh (dung nhu slide: doc canh roi theo dong).
    //   - Z-buffer, Phong, texture: noi suy dung phoi canh, trong so p_i = (b_i / z_i) / sum(b_j / z_j)
    //     (xem interpolationWeights). Do sau z = sum p_i z_i = 1 / sum(b_j / z_j).
    const bool useDepth = s.zBuffer && !s.painter;                  // Painter thay cho Z-buffer
    const bool lit = ctx.material && ctx.lights;
    const bool phong = s.shading == ShadingMode::Phong && lit;
    const bool gouraud = s.shading == ShadingMode::Gouraud;
    const bool perPixel = texture || useDepth || phong || gouraud;  // Flat khong texture, khong Z: to mot mau
    const double ox = tri[0].x, oy = tri[0].y;
    const double e1x = double(tri[1].x) - ox, e1y = double(tri[1].y) - oy;
    const double e2x = double(tri[2].x) - ox, e2y = double(tri[2].y) - oy;
    const double triArea = e1x * e2y - e1y * e2x;
    const double invArea = triArea != 0.0 ? 1.0 / triArea : 0.0;
    double invZ[3];
    for (int i = 0; i < 3; ++i) invZ[i] = 1.0 / std::max(double(tri[i].z), 1e-6);

    const auto shadePixel = [&](int column, int row) {
        if (!perPixel) { fb.setPixel(column, row, tri[0].color); return; }
        const double dx = column + 0.5 - ox, dy = row + 0.5 - oy;
        const double b1 = (dx * e2y - dy * e2x) * invArea;
        const double b2 = (e1x * dy - e1y * dx) * invArea;
        const double b[3] = {1 - b1 - b2, b1, b2};
        double w[3] = {b[0], b[1], b[2]};
        if (perspective) {
            const double sum = b[0] * invZ[0] + b[1] * invZ[1] + b[2] * invZ[2];
            for (int i = 0; i < 3; ++i) w[i] = b[i] * invZ[i] / sum;
        }
        auto lerp = [&](const double k[3], auto get) { return get(tri[0]) * float(k[0]) + get(tri[1]) * float(k[1]) + get(tri[2]) * float(k[2]); };

        if (useDepth) {  // Z-buffer (slide CG.6): chi ve khi gan hon diem da ve tai pixel nay
            const float z = static_cast<float>(w[0] * tri[0].z + w[1] * tri[1].z + w[2] * tri[2].z);
            float& stored = fb.depthAt(column, row);
            if (z >= stored) return;
            stored = z;
        }
        Vec3 texel{1, 1, 1};
        if (texture) {
            const float u = static_cast<float>(w[0] * tri[0].u + w[1] * tri[1].u + w[2] * tri[2].u);
            const float v = static_cast<float>(w[0] * tri[0].v + w[1] * tri[1].v + w[2] * tri[2].v);
            texel = texture->sample(u, v);
        }
        Vec3 color;
        if (phong) {
            // Phong (slide CG.6): noi suy PHAP TUYEN (va vi tri) roi tinh chieu sang tai tung pixel.
            // Texture thay mau nen (ka, kd); phan phan xa guong ks giu nguyen de vet sang van trang.
            const Material& m = *ctx.material;
            const Vec3 N = lerp(w, [](const ScreenVertex& v) { return v.normal; });
            const Vec3 P = lerp(w, [](const ScreenVertex& v) { return v.world; });
            color = phongTerms(P, N, ctx.eye, *ctx.lights, mul(m.ka, texel), mul(m.kd, texel), m.ks, m.shininess, s.blinnPhong);
        } else {
            color = gouraud ? lerp(b, [](const ScreenVertex& v) { return v.color; }) : tri[0].color;
            color = mul(color, texel);
        }
        fb.setPixel(column, row, color);
    };

    const ScreenVertex* sorted[3] = {&tri[0], &tri[1], &tri[2]};
    std::sort(sorted, sorted + 3, [](const ScreenVertex* left, const ScreenVertex* right) {
        return left->y < right->y;
    });
    const double area = (double(sorted[1]->x) - sorted[0]->x) * (double(sorted[2]->y) - sorted[0]->y)
                      - (double(sorted[1]->y) - sorted[0]->y) * (double(sorted[2]->x) - sorted[0]->x);
    if (area == 0.0) return;

    const auto fillHalf = [&](double topY, double bottomY, double topLeft, double topRight,
                              double bottomLeft, double bottomRight) {
        if (topY >= bottomY) return;
        const double leftStep = (bottomLeft - topLeft) / (bottomY - topY);
        const double rightStep = (bottomRight - topRight) / (bottomY - topY);
        // Chi to cac hang trong [rowBegin, rowEnd): moi luong to mot dai hang rieng (DHMTPTB3-45).
        const double lo = std::max(0, rowBegin), hi = std::min(fb.height, rowEnd);
        const int firstRow = static_cast<int>(std::clamp(std::ceil(topY - 0.5), lo, std::max(lo, hi)));
        const int endRow = static_cast<int>(std::clamp(std::ceil(bottomY - 0.5), lo, std::max(lo, hi)));
        for (int row = firstRow; row < endRow; ++row) {
            // Tinh mep trai/phai truc tiep tai tam hang (khong cong don left += leftStep): cong don
            // tich luy sai so float, pixel nam dung tren canh co the lech sang 1 ben tuy hang bat dau
            // -> to theo dai hang (nhieu luong) se khac to ca tam giac.
            const double left = topLeft + (row + 0.5 - topY) * leftStep;
            const double right = topRight + (row + 0.5 - topY) * rightStep;
            const int firstColumn = static_cast<int>(std::clamp(std::ceil(left - 0.5), 0.0, double(fb.width)));
            const int endColumn = static_cast<int>(std::clamp(std::ceil(right - 0.5), 0.0, double(fb.width)));
            for (int column = firstColumn; column < endColumn; ++column) shadePixel(column, row);
        }
    };

    const double splitX = sorted[0]->x + (double(sorted[1]->y) - sorted[0]->y)
                       * (double(sorted[2]->x) - sorted[0]->x) / (double(sorted[2]->y) - sorted[0]->y);
    const double middleLeft = std::min(double(sorted[1]->x), splitX);
    const double middleRight = std::max(double(sorted[1]->x), splitX);
    fillHalf(sorted[0]->y, sorted[1]->y, sorted[0]->x, sorted[0]->x, middleLeft, middleRight);
    fillHalf(sorted[1]->y, sorted[2]->y, middleLeft, middleRight, sorted[2]->x, sorted[2]->x);
}

// Bresenham (Lab 01, slide CG.2): ve doan thang chi bang cong/tru so nguyen.
// Chon truc "chinh" theo do doc:
//   |dy| <= |dx| (4 huong gan ngang): moi buoc x tang/giam 1, y doi khi sai so tich luy vuot 1/2.
//   |dy| >  |dx| (4 huong gan dung) : vai tro x, y doi cho.
// sx, sy = +-1 cho phep di ve ca 2 phia -> du 8 truong hop huong ma khong lap code.
// Sai so nhan 2*dx de khong co phan so: p = 2dy - dx; p < 0 -> giu y, p += 2dy; nguoc lai y += sy, p += 2(dy - dx).
void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Vec3 color) {
    const int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    const int sx = x1 >= x0 ? 1 : -1, sy = y1 >= y0 ? 1 : -1;
    int x = x0, y = y0;
    if (dy <= dx) {                       // truc chinh x
        int p = 2 * dy - dx;
        for (int i = 0; i <= dx; ++i) {
            fb.setPixel(x, y, color);
            x += sx;
            if (p < 0) p += 2 * dy;
            else { y += sy; p += 2 * (dy - dx); }
        }
    } else {                              // truc chinh y
        int p = 2 * dx - dy;
        for (int i = 0; i <= dy; ++i) {
            fb.setPixel(x, y, color);
            y += sy;
            if (p < 0) p += 2 * dx;
            else { x += sx; p += 2 * (dx - dy); }
        }
    }
}

// Liang–Barsky: cat doan thang (a -> b) theo hinh chu nhat [0, w-1] x [0, h-1] truoc khi Bresenham.
// Can vi canh gan mat phang gan co the chieu ra toa do rat lon (hang trieu pixel): khong cat thi
// vong lap Bresenham rat dai, va ep float qua lon sang int la loi.
// Diem tren doan: P(t) = a + t (b - a), t in [0, 1]. Moi canh khung cho mot bat dang thuc p_k t <= q_k.
bool clipLineToRect(float& ax, float& ay, float& bx, float& by, float w, float h) {
    // Tinh bang double: toa do co the toi 1e6 pixel, float chi giu ~7 chu so.
    const double x0 = ax, y0 = ay, dx = double(bx) - x0, dy = double(by) - y0;
    const double p[4] = {-dx, dx, -dy, dy};
    const double q[4] = {x0, w - 1 - x0, y0, h - 1 - y0};
    double t0 = 0, t1 = 1;
    for (int k = 0; k < 4; ++k) {
        if (p[k] == 0) {
            if (q[k] < 0) return false;    // song song voi canh khung va nam ngoai
            continue;
        }
        const double t = q[k] / p[k];
        if (p[k] < 0) t0 = std::max(t0, t);  // di vao
        else t1 = std::min(t1, t);           // di ra
        if (t0 > t1) return false;
    }
    ax = float(x0 + t0 * dx); ay = float(y0 + t0 * dy);
    bx = float(x0 + t1 * dx); by = float(y0 + t1 * dy);
    return true;
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
        // Painter (slide CG.6): ve tu XA den GAN, vat gan ve sau de len vat xa; khong can Z-buffer.
        // Do sau moi tam giac = trung binh z 3 dinh. SAI khi 2 tam giac cat nhau hoac che vong tron
        // (khong co thu tu nao dung) -> xem canh qua cau xuyen tam phang (phim K).
        // stable_sort: tam giac cung do sau giu thu tu cu -> anh khong nhay giua cac khung hinh.
        std::stable_sort(queue.begin(), queue.end(), [](const Pending& a, const Pending& b) { return a.depth > b.depth; });
    }

    // ---- Song song hoa (DHMTPTB3-45) ----
    // Buoc 1: tinh mau dinh (Flat/Gouraud) cho tung tam giac — chia danh sach tam giac cho cac luong,
    //         moi luong chi ghi vao tam giac cua minh.
    // Buoc 2: to — chia MAN HINH thanh cac dai hang xen ke (luong k to dai k, k + T, k + 2T, ...).
    //         Moi pixel chi thuoc 1 luong nen khong tranh nhau ghi mau / Z-buffer, va trong moi dai
    //         tam giac van ve dung thu tu cua queue (Painter van dung).
    const int threads = std::clamp(settings.threads, 1, 16);
    auto parallel = [](int count, auto&& job) {  // job(k) chay tren luong k, k = 0..count-1
        if (count <= 1) { job(0); return; }
        std::vector<std::thread> pool;
        for (int k = 1; k < count; ++k) pool.emplace_back(job, k);
        job(0);
        for (std::thread& t : pool) t.join();
    };

    parallel(threads, [&](int k) {
        const std::size_t n = queue.size();
        for (std::size_t i = n * k / threads; i < n * (k + 1) / threads; ++i) {
            Pending& p = queue[i];
            p.v[0].color = p.mat->kd;
            if (settings.shading == ShadingMode::Flat) {
                const Vec3 c = (p.v[0].world + p.v[1].world + p.v[2].world) / 3.0f;
                const Vec3 nrm = normalize(cross(p.v[1].world - p.v[0].world, p.v[2].world - p.v[0].world));
                p.v[0].color = shadePhong(c, nrm, eye, lights, *p.mat, settings.blinnPhong);
            } else if (settings.shading == ShadingMode::Gouraud) {
                for (ScreenVertex& v : p.v) v.color = shadePhong(v.world, v.normal, eye, lights, *p.mat, settings.blinnPhong);
            }
        }
    });

    ShadeContext base;
    base.lights = &lights; base.eye = eye;
    base.perspective = camera.projection == Projection::Perspective;
    if (settings.shading == ShadingMode::Wireframe) {
        // Khung day: Bresenham ve ca doan thang qua nhieu dai hang -> lam tuan tu.
        const Vec3 line{0.9f, 0.9f, 0.9f};
        for (const Pending& p : queue)
            for (int e = 0; e < 3; ++e) {
                const ScreenVertex& a = p.v[e];
                const ScreenVertex& b = p.v[(e + 1) % 3];
                float ax = a.x, ay = a.y, bx = b.x, by = b.y;
                if (clipLineToRect(ax, ay, bx, by, float(fb.width), float(fb.height)))
                    drawLineBresenham(fb, int(std::lround(ax)), int(std::lround(ay)), int(std::lround(bx)), int(std::lround(by)), line);
            }
    } else {
        // Dai hang cao ~ height / (4T): du nho de chia deu viec (san o duoi nang hon tran), du lon de
        // moi luong khong phai duyet queue qua nhieu lan.
        const int bandCount = threads == 1 ? 1 : threads * 4;
        const int bandH = (fb.height + bandCount - 1) / bandCount;
        parallel(threads, [&](int k) {
            ShadeContext ctx = base;
            for (int band = k; band < bandCount; band += threads) {
                const int r0 = band * bandH, r1 = std::min(fb.height, r0 + bandH);
                for (const Pending& p : queue) {
                    const float top = std::min({p.v[0].y, p.v[1].y, p.v[2].y});
                    const float bottom = std::max({p.v[0].y, p.v[1].y, p.v[2].y});
                    if (bottom < r0 - 1 || top > r1 + 1) continue;  // tam giac khong cham dai nay
                    ctx.material = p.mat;
                    fillTriangle(fb, p.v, ctx, settings, r0, r1);
                }
            }
        });
    }
    stats.trianglesDrawn = static_cast<int>(queue.size());
    stats.msRaster = msSince(t1);

    // Phim V (DHMTPTB3-32): hien mang do sau thay cho anh mau. Gan = sang, xa = toi,
    // pixel khong co vat = den. Chuan hoa theo khoang [gan nhat, xa nhat] dang co trong khung.
    if (settings.showDepth) {
        float lo = std::numeric_limits<float>::infinity(), hi = 0;
        for (float d : fb.depth)
            if (std::isfinite(d)) { lo = std::min(lo, d); hi = std::max(hi, d); }
        const float range = hi > lo ? hi - lo : 1.0f;
        for (int y = 0; y < fb.height; ++y)
            for (int x = 0; x < fb.width; ++x) {
                const float d = fb.depthAt(x, y);
                const float g = std::isfinite(d) ? 1.0f - 0.85f * (d - lo) / range : 0.0f;
                fb.setPixel(x, y, {g, g, g});
            }
    }
}
