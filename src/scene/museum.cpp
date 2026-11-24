// PHAN B — BAO TANG MO RONG (Hao). Mat bang, tuong sinh tu luoi, tranh, tu kinh, co vat.
#include "museum.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "modeling/modeling.hpp"
#include "texture_loader.hpp"

// ============================================================================
// 1. LUOI DI LAI VA VA CHAM
// ============================================================================
int WalkGrid::zoneAt(float x, float z) const {
    const int i = static_cast<int>(std::floor((x - x0) / cell));
    const int j = static_cast<int>(std::floor((z - z0) / cell));
    if (i < 0 || j < 0 || i >= nx || j >= nz) return -1;
    return zone[static_cast<std::size_t>(j) * nx + i];
}

namespace {

// Moi o (i, j) phu boi hinh vuong tam (x, z), nua canh r, deu thoa `ok`?
template <class F>
bool everyCell(const WalkGrid& g, float x, float z, float r, F ok) {
    const int i0 = static_cast<int>(std::floor((x - r - g.x0) / g.cell));
    const int i1 = static_cast<int>(std::floor((x + r - g.x0) / g.cell));
    const int j0 = static_cast<int>(std::floor((z - r - g.z0) / g.cell));
    const int j1 = static_cast<int>(std::floor((z + r - g.z0) / g.cell));
    for (int j = j0; j <= j1; ++j)
        for (int i = i0; i <= i1; ++i) {
            if (i < 0 || j < 0 || i >= g.nx || j >= g.nz) return false;
            if (!ok(static_cast<std::size_t>(j) * g.nx + i)) return false;
        }
    return true;
}

}  // namespace

bool WalkGrid::insideWalls(float x, float z, float margin) const {
    return everyCell(*this, x, z, margin, [&](std::size_t k) { return zone[k] >= 0; });
}

bool WalkGrid::free(float x, float z, float radius) const {
    return everyCell(*this, x, z, radius, [&](std::size_t k) { return zone[k] >= 0 && !blocked[k]; });
}

void WalkGrid::block(const Rect& r) {
    for (int j = 0; j < nz; ++j)
        for (int i = 0; i < nx; ++i) {
            const float cx0 = x0 + i * cell, cz0 = z0 + j * cell;
            if (cx0 < r.x1 && cx0 + cell > r.x0 && cz0 < r.z1 && cz0 + cell > r.z0)
                blocked[static_cast<std::size_t>(j) * nx + i] = 1;
        }
}

// Chia buoc lon thanh buoc <= 5 cm (khong "nhay" qua vat can mong). Moi buoc: thu di ca 2 truc;
// bi chan thi thu rieng x roi rieng z -> nhan vat truot doc tuong thay vi dung khung lai.
Vec3 WalkGrid::slide(Vec3 from, Vec3 delta, float radius) const {
    const float len = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    const int steps = std::max(1, static_cast<int>(std::ceil(len / 0.05f)));
    const float dx = delta.x / steps, dz = delta.z / steps;
    Vec3 p = from;
    for (int k = 0; k < steps; ++k) {
        if (free(p.x + dx, p.z + dz, radius)) { p.x += dx; p.z += dz; }
        else if (free(p.x + dx, p.z, radius)) p.x += dx;
        else if (free(p.x, p.z + dz, radius)) p.z += dz;
    }
    return p;
}

// Lo xo camera (spring arm): buoc 5 cm tu dau nhan vat ve phia camera, dung o buoc cuoi con
// nam trong tuong va giua san - tran. Bo qua vat can thap (buc): camera duoc bay qua tren buc.
float springArm(const WalkGrid& grid, Vec3 target, Vec3 dir, float wanted, float ceiling, float margin) {
    const float step = 0.05f;
    float d = 0;
    while (d + step <= wanted) {
        const Vec3 p = target + dir * (d + step);
        if (!grid.insideWalls(p.x, p.z, margin) || p.y < 0.15f || p.y > ceiling - 0.15f) break;
        d += step;
    }
    return d;
}

int Museum::add(SceneObject object, int zone, ObjKind k, bool shadow, std::string description, int stand) {
    objects.push_back(std::move(object));
    zoneOf.push_back(zone);
    kind.push_back(k);
    castsShadow.push_back(shadow);
    standOf.push_back(stand);
    info.push_back(std::move(description));
    center.push_back({});
    radius.push_back(0);
    const int i = static_cast<int>(objects.size()) - 1;
    updateBounds(i);
    return i;
}

void Museum::updateBounds(int i) {
    worldBounds(objects[i], center[i], radius[i]);
}

int Museum::paintingCount() const {
    return static_cast<int>(std::count(kind.begin(), kind.end(), ObjKind::Painting));
}

// ============================================================================
// 2. TIEN ICH DUNG LUOI
// ============================================================================
namespace {

Material mat(const char* name, Vec3 kd, Vec3 ks, float shininess) {
    Material m;
    m.name = name;
    m.kd = kd;
    m.ka = kd * 0.15f;
    m.ks = ks;
    m.shininess = shininess;
    return m;
}

// Gan anh cho vat lieu. Khong nap duoc: giu mau cu, ghi canh bao (canh van chay).
bool attach(Material& m, const std::string& path, std::vector<std::string>& warnings) {
    std::string err;
    std::shared_ptr<const Texture> tex = loadTexture(path, err);
    if (!tex) { warnings.push_back(err); return false; }
    m.texture = std::move(tex);
    return true;
}

// Ghep luoi `src` (bien doi bang M: chi tinh tien + quay) vao `dst`. Hai luoi cung vat lieu.
void append(Mesh& dst, const Mesh& src, const Mat4& M) {
    const int base = static_cast<int>(dst.vertices.size());
    for (Vertex v : src.vertices) {
        v.position = transformPoint(M, v.position);
        v.normal = normalize(transformDir(M, v.normal));
        dst.vertices.push_back(v);
    }
    for (const auto& t : src.triangles) dst.triangles.push_back({base + t[0], base + t[1], base + t[2]});
}

// Mat phang chu nhat origin + s du + t dv (s, t in [0, 1]) chia nu x nv o, phap tuyen `normal`.
// Chia nho (khoang 1 m / o) de Gouraud va anh sang tren tuong, san dai van muot.
// Chieu dinh chon theo `normal` (nguoc chieu kim dong ho nhin tu phia normal).
template <class UV>
void panel(Mesh& m, Vec3 origin, Vec3 du, Vec3 dv, int nu, int nv, Vec3 normal, UV uv) {
    const int base = static_cast<int>(m.vertices.size());
    for (int j = 0; j <= nv; ++j)
        for (int i = 0; i <= nu; ++i) {
            Vertex v;
            v.position = origin + du * (static_cast<float>(i) / nu) + dv * (static_cast<float>(j) / nv);
            v.normal = normal;
            uv(v.position, v.u, v.v);
            m.vertices.push_back(v);
        }
    const bool flip = dot(cross(du, dv), normal) < 0;
    for (int j = 0; j < nv; ++j)
        for (int i = 0; i < nu; ++i) {
            const int a = base + j * (nu + 1) + i, b = a + 1, c = a + nu + 2, d = a + nu + 1;
            if (!flip) { m.triangles.push_back({a, b, c}); m.triangles.push_back({a, c, d}); }
            else { m.triangles.push_back({a, c, b}); m.triangles.push_back({a, d, c}); }
        }
}

// San: (u, v) theo met trong the gioi, giong san phong cu va luoi bong (shadow.cpp) -> van go
// noi lien giua cac khu, bong giu dung van san.
void floorUV(Vec3 p, float& u, float& v) {
    u = (p.x + kRoomHalfX) * 0.5f;
    v = (p.z + kRoomHalfZ) * 0.5f;
}

int pieces(float len) { return std::max(1, static_cast<int>(std::ceil(len - 1e-3f))); }

SceneObject placed(Mesh mesh, const Mat4& model) {
    SceneObject o;
    o.mesh = std::move(mesh);
    o.model = model;
    return o;
}

// ============================================================================
// 3. VO BAO TANG: SAN, TRAN THEO KHU; TUONG SINH TU LUOI
// ============================================================================
// Thanh hop dai doc theo truc x hoac z (nep tuong, dam cua...): tam `c`, huong `dir` (truc toa do).
void bar(Mesh& m, Vec3 c, Vec3 dir, float halfLen, float halfY, float halfDepth, const Material& mat) {
    const Vec3 half = std::fabs(dir.x) > 0.5f ? Vec3{halfLen, halfY, halfDepth} : Vec3{halfDepth, halfY, halfLen};
    append(m, makeBox(half, mat), translate(c));
}

// Vat lieu trang tri dung chung cho moi khu.
struct ShellStyle {
    std::vector<Material> walls;  // giay dan tuong theo khu
    Material ceiling;
    Material dado;                // op go chan tuong
    Material trim;                // nep chan, nep ngang, phao tran
    Material portal;              // cot va dam cua
    Material gilt;                // chi vang tren dam cua
};

void buildShell(Museum& mu, const ShellStyle& style) {
    WalkGrid& g = mu.grid;
    const float H = kRoomHeight;
    const int zoneCount = static_cast<int>(mu.zones.size());
    std::vector<Mesh> walls(zoneCount), dado(zoneCount), trim(zoneCount), portal(zoneCount), gilt(zoneCount);
    for (int z = 0; z < zoneCount; ++z) {
        dado[z].material = style.dado;
        trim[z].material = style.trim;
        portal[z].material = style.portal;
        gilt[z].material = style.gilt;
    }

    // Doan tuong tai duong luoi: o 2 ben, mot ben di duoc (khu `zone`), mot ben khong.
    // Kem trang tri: op go cao 0,9 m (lech 1,2 cm ra truoc tuong), nep chan, nep ngang tren op go, phao tran.
    auto wallAt = [&](int zone, Vec3 a, Vec3 b, Vec3 normal) {
        const Vec3 along = b - a;
        const float len = length(along);
        const Vec3 dir = along / len, mid = (a + b) * 0.5f;
        // Ngoai troi: mat tien toa nha (tuong giap phia trong bao tang, z = cua chinh) cao bang nha;
        // tuong rao quanh san chi cao 1,1 m -> nhin qua thay bau troi.
        const bool outdoor = mu.zones[zone].outdoor;
        const float h = (outdoor && std::fabs(a.z - kLobby.z1) > 0.01f) ? 1.1f : H;
        panel(walls[zone], a, along, Vec3{0, h, 0}, pieces(len), outdoor && h < H ? 1 : 3, normal,
              [&](Vec3 p, float& u, float& v) { u = dot(p - a, dir) * 0.5f; v = p.y * 0.5f; });
        if (outdoor) {  // mat tien / tuong rao: go chan va go dinh (mat tren tuong rao phu bang go dinh)
            bar(portal[zone], mid + normal * 0.06f + Vec3{0, 0.2f, 0}, dir, len / 2, 0.2f, 0.06f, style.portal);
            bar(trim[zone], mid + normal * 0.04f + Vec3{0, h - 0.06f, 0}, dir, len / 2 + 0.08f, 0.08f, 0.12f, style.trim);
            return;
        }
        panel(dado[zone], a + normal * 0.012f, along, Vec3{0, 0.9f, 0}, pieces(len), 1, normal,
              [&](Vec3 p, float& u, float& v) { u = dot(p - a, dir); v = p.y; });
        bar(trim[zone], mid + normal * 0.022f + Vec3{0, 0.07f, 0}, dir, len / 2, 0.07f, 0.022f, style.trim);
        bar(trim[zone], mid + normal * 0.03f + Vec3{0, 0.92f, 0}, dir, len / 2, 0.025f, 0.03f, style.trim);
        bar(trim[zone], mid + normal * 0.05f + Vec3{0, H - 0.06f, 0}, dir, len / 2, 0.06f, 0.05f, style.trim);
    };
    // Cho 2 khu giap nhau (cua thong): 2 cot o 2 dau, dam ngang tu 2,75 m len tran, chi vang tren dam.
    auto doorAt = [&](int zone, Vec3 a, Vec3 b) {
        const Vec3 along = b - a;
        const float len = length(along);
        const Vec3 dir = along / len, mid = (a + b) * 0.5f;
        const float top = 2.75f;
        bar(portal[zone], mid + Vec3{0, (top + H) / 2, 0}, dir, len / 2 + 0.25f, (H - top) / 2, 0.2f, style.portal);
        bar(gilt[zone], mid + Vec3{0, top + 0.18f, 0}, dir, len / 2 + 0.26f, 0.035f, 0.21f, style.gilt);
        for (const Vec3 end : {a, b}) {
            bar(portal[zone], end + Vec3{0, top / 2, 0}, dir, 0.18f, top / 2, 0.18f, style.portal);
            bar(gilt[zone], end + Vec3{0, top - 0.07f, 0}, dir, 0.24f, 0.07f, 0.24f, style.gilt);  // dau cot
            bar(portal[zone], end + Vec3{0, 0.08f, 0}, dir, 0.24f, 0.08f, 0.24f, style.portal);   // chan cot
            g.block({end.x - 0.26f, end.z - 0.26f, end.x + 0.26f, end.z + 0.26f});
        }
    };
    auto cellZone = [&](int i, int j) { return (i < 0 || j < 0 || i >= g.nx || j >= g.nz) ? -1 : g.zone[static_cast<std::size_t>(j) * g.nx + i]; };

    // Duong luoi doc (x = hang so): so o trai (i-1, j) va o phai (i, j). Gop cac o lien tiep thanh 1 doan.
    for (int i = 0; i <= g.nx; ++i) {
        const float x = g.x0 + i * g.cell;
        for (int j = 0; j < g.nz;) {
            const int L = cellZone(i - 1, j), R = cellZone(i, j);
            if (L == R || (L < 0 && R < 0)) { ++j; continue; }
            int k = j;
            while (k < g.nz && cellZone(i - 1, k) == L && cellZone(i, k) == R) ++k;
            const Vec3 a{x, 0, g.z0 + j * g.cell}, b{x, 0, g.z0 + k * g.cell};
            if (L >= 0 && R >= 0) doorAt(std::min(L, R), a, b);
            else wallAt(L >= 0 ? L : R, a, b, Vec3{R >= 0 ? 1.0f : -1.0f, 0, 0});  // huong ve phia di duoc
            j = k;
        }
    }
    // Duong luoi ngang (z = hang so).
    for (int j = 0; j <= g.nz; ++j) {
        const float z = g.z0 + j * g.cell;
        for (int i = 0; i < g.nx;) {
            const int B = cellZone(i, j - 1), F = cellZone(i, j);
            if (B == F || (B < 0 && F < 0)) { ++i; continue; }
            int k = i;
            while (k < g.nx && cellZone(k, j - 1) == B && cellZone(k, j) == F) ++k;
            const Vec3 a{g.x0 + i * g.cell, 0, z}, b{g.x0 + k * g.cell, 0, z};
            if (B >= 0 && F >= 0) doorAt(std::min(B, F), a, b);
            else wallAt(B >= 0 ? B : F, a, b, Vec3{0, 0, F >= 0 ? 1.0f : -1.0f});
            i = k;
        }
    }

    for (int z = 0; z < zoneCount; ++z) {
        const Rect& r = mu.zones[z].area;
        const Vec3 o{r.x0, 0, r.z0}, du{r.x1 - r.x0, 0, 0}, dv{0, 0, r.z1 - r.z0};
        Mesh floor;
        floor.name = "san " + mu.zones[z].name;
        floor.material = mu.zones[z].floor;
        panel(floor, o, du, dv, pieces(du.x), pieces(dv.z), Vec3{0, 1, 0}, floorUV);
        mu.add(placed(std::move(floor), Mat4::identity()), z, ObjKind::Floor, false);

        walls[z].name = "tuong " + mu.zones[z].name;
        walls[z].material = style.walls[z];
        mu.add(placed(std::move(walls[z]), Mat4::identity()), z, ObjKind::Wall, false, "Giay dan tuong hoa van (texture tu sinh)");
        dado[z].name = "op go chan tuong";
        trim[z].name = "nep, phao trang tri";
        portal[z].name = "cot va dam cua";
        gilt[z].name = "chi vang cot, dam";
        for (Mesh* m : {&dado[z], &trim[z], &portal[z], &gilt[z]})
            if (!m->triangles.empty()) mu.add(placed(std::move(*m), Mat4::identity()), z, ObjKind::Decor, false);

        if (mu.zones[z].outdoor) continue;  // ngoai troi: khong tran, nhin len thay bau troi (mau nen)
        Mesh ceil;
        ceil.name = "tran " + mu.zones[z].name;
        ceil.material = style.ceiling;
        panel(ceil, o + Vec3{0, H, 0}, du, dv, pieces(du.x), pieces(dv.z), Vec3{0, -1, 0}, floorUV);
        mu.add(placed(std::move(ceil), Mat4::identity()), z, ObjKind::Ceiling, false);
    }
}

// ============================================================================
// 4. TRANH TREO HANH LANG (anh public domain, nguon trong assets/README.md)
// ============================================================================
struct PaintingDef {
    const char* file;    // assets/paintings/<file>
    const char* title;
    const char* author;
    int zone;
    float x;             // vi tri doc hanh lang
    bool northWall;      // true: tuong z nho (mat huong +z); false: tuong z lon (mat huong -z)
};

void addPainting(Museum& mu, const PaintingDef& d, const Material& frameMat) {
    Material canvas = mat("tranh", {0.92f, 0.92f, 0.92f}, {0.04f, 0.04f, 0.04f}, 8);
    canvas.ka = {0.22f, 0.22f, 0.22f};  // tranh van doc duoc o goc khuat den
    if (!attach(canvas, std::string("assets/paintings/") + d.file, mu.warnings))
        canvas.texture = makeCheckerTexture(64, 8, {0.8f, 0.8f, 0.8f}, {0.3f, 0.3f, 0.35f});
    const Texture& tex = *canvas.texture;

    // Khung toi da 1,5 m x 1,15 m, giu ti le anh.
    const float aspect = static_cast<float>(tex.width) / std::max(1, tex.height);
    const float w = std::min(1.5f, 1.15f * aspect), h = w / aspect;

    // Mat tranh: nam trong mat phang z = 0,03 (giua be day khung), huong +z.
    // Thu (u, v) vao trong nua texel de lay mau song tuyen khong lan sang canh ben kia (repeat).
    const float iu = 0.5f / tex.width, iv = 0.5f / tex.height;
    Mesh face;
    face.name = d.title;
    face.material = canvas;
    panel(face, {-w / 2, -h / 2, 0.03f}, {w, 0, 0}, {0, h, 0}, 1, 1, Vec3{0, 0, 1}, [&](Vec3 p, float& u, float& v) {
        u = iu + (p.x / w + 0.5f) * (1 - 2 * iu);
        v = iv + (p.y / h + 0.5f) * (1 - 2 * iv);
    });

    // Khung: 4 thanh hop ghep thanh 1 luoi.
    const float t = 0.07f, depth = 0.025f;
    Mesh frame;
    frame.name = "khung tranh";
    frame.material = frameMat;
    append(frame, makeBox({w / 2 + t, t / 2, depth}, frameMat), translate({0, h / 2 + t / 2, depth}));
    append(frame, makeBox({w / 2 + t, t / 2, depth}, frameMat), translate({0, -h / 2 - t / 2, depth}));
    append(frame, makeBox({t / 2, h / 2, depth}, frameMat), translate({-w / 2 - t / 2, 0, depth}));
    append(frame, makeBox({t / 2, h / 2, depth}, frameMat), translate({w / 2 + t / 2, 0, depth}));

    const Rect& r = mu.zones[d.zone].area;
    const float wallZ = d.northWall ? r.z0 : r.z1;
    const Mat4 model = translate({d.x, 1.65f, wallZ}) * rotateY(d.northWall ? 0.0f : kPi);  // quay de mat tranh nhin vao hanh lang
    char text[200];
    std::snprintf(text, sizeof text, "%s - %s", d.title, d.author);
    const int fi = mu.add(placed(std::move(frame), model), d.zone, ObjKind::Frame, false, text);
    mu.add(placed(std::move(face), model), d.zone, ObjKind::Painting, false, text, fi);
}

// ============================================================================
// 5. CO VAT (mat tron xoay tu profile Bezier — dung lai revolve() cua phan A)
// ============================================================================
// Toa do profile (r, y) theo met, day vat o y = 0. Thanh ngoai di tu duoi len, mat ngoai ben phai.
Vec3 P(float x, float y) { return {x, y, 0}; }

// Trong dong (mo phong trong Ngoc Lu, van hoa Dong Son): mat trong o tren cung. Texture dong_son.png:
// 20 % tren cua anh phu mat trong (v = 0,8 .. 1; hang tren cung = tam mat trong) — ngoi sao 14 canh
// va cac vanh hoa van; phan con lai phu than trong.
Mesh bronzeDrum(const Material& m) {
    Profile p;
    p.segments = {
        bezierLine(P(0, 0), P(0.33f, 0)),
        {P(0.33f, 0), P(0.335f, 0.06f), P(0.31f, 0.13f), P(0.30f, 0.20f)},   // chan trong loe
        {P(0.30f, 0.20f), P(0.29f, 0.33f), P(0.41f, 0.42f), P(0.385f, 0.55f)},  // tang trong phinh
        {P(0.385f, 0.55f), P(0.38f, 0.58f), P(0.39f, 0.60f), P(0.39f, 0.62f)},  // mep mat trong
        bezierLine(P(0.39f, 0.62f), P(0, 0.62f)),                             // mat trong
    };
    Mesh mesh = revolve(p, 10, 64, m);
    mesh.name = "trong dong";
    return mesh;
}

Mesh blueWhiteVase(const Material& m) {
    Profile p;
    p.segments = {
        bezierLine(P(0, 0), P(0.07f, 0)),
        bezierLine(P(0.07f, 0), P(0.07f, 0.02f)),
        {P(0.07f, 0.02f), P(0.10f, 0.06f), P(0.19f, 0.20f), P(0.15f, 0.33f)},
        {P(0.15f, 0.33f), P(0.12f, 0.37f), P(0.05f, 0.37f), P(0.045f, 0.40f)},
        {P(0.045f, 0.40f), P(0.045f, 0.41f), P(0.06f, 0.415f), P(0.06f, 0.42f)},
        bezierLine(P(0.06f, 0.42f), P(0.05f, 0.42f)),
        {P(0.05f, 0.42f), P(0.035f, 0.40f), P(0.035f, 0.38f), P(0.03f, 0.36f)},
        bezierLine(P(0.03f, 0.36f), P(0, 0.36f)),
    };
    Mesh mesh = revolve(p, 8, 48, m);
    mesh.name = "binh gom hoa lam";
    return mesh;
}

Mesh incenseBurner(const Material& m) {
    Profile p;
    p.segments = {
        bezierLine(P(0, 0.08f), P(0.10f, 0.08f)),
        {P(0.10f, 0.08f), P(0.16f, 0.085f), P(0.18f, 0.16f), P(0.16f, 0.22f)},
        bezierLine(P(0.16f, 0.22f), P(0.18f, 0.23f)),
        bezierLine(P(0.18f, 0.23f), P(0.165f, 0.23f)),
        {P(0.165f, 0.23f), P(0.15f, 0.20f), P(0.15f, 0.13f), P(0.09f, 0.11f)},
        bezierLine(P(0.09f, 0.11f), P(0, 0.11f)),
    };
    Mesh mesh = revolve(p, 8, 48, m);
    for (int k = 0; k < 3; ++k) {  // 3 chan tru, cach nhau 120 do
        const float a = 2 * kPi * k / 3;
        append(mesh, makeCylinder(0.018f, 0.09f, 12, m), translate({0.075f * std::cos(a), 0, 0.075f * std::sin(a)}));
    }
    for (int s = -1; s <= 1; s += 2)  // 2 quai: xuyen dung tren mieng
        append(mesh, makeTorus(0.035f, 0.009f, 12, 20, m), translate({s * 0.17f, 0.26f, 0}) * rotateX(kPi / 2));
    mesh.name = "lu huong dong ba chan";
    return mesh;
}

Mesh jadeCup(const Material& m) {
    Profile saucer;
    saucer.segments = {
        bezierLine(P(0, 0), P(0.09f, 0)),
        bezierLine(P(0.09f, 0), P(0.13f, 0.02f)),
        bezierLine(P(0.13f, 0.02f), P(0.122f, 0.022f)),
        bezierLine(P(0.122f, 0.022f), P(0, 0.008f)),
    };
    Profile cup;
    cup.segments = {
        bezierLine(P(0, 0), P(0.035f, 0)),
        bezierLine(P(0.035f, 0), P(0.035f, 0.012f)),
        {P(0.035f, 0.012f), P(0.06f, 0.02f), P(0.075f, 0.05f), P(0.08f, 0.085f)},
        bezierLine(P(0.08f, 0.085f), P(0.074f, 0.085f)),
        {P(0.074f, 0.085f), P(0.068f, 0.05f), P(0.045f, 0.026f), P(0.02f, 0.022f)},
        bezierLine(P(0.02f, 0.022f), P(0, 0.022f)),
    };
    Mesh mesh = revolve(saucer, 4, 48, m);
    append(mesh, revolve(cup, 8, 48, m), translate({0, 0.008f, 0}));
    mesh.name = "chen ngoc bich";
    return mesh;
}

Mesh goldGourd(const Material& m) {
    Profile p;
    p.segments = {
        bezierLine(P(0, 0), P(0.07f, 0)),
        {P(0.07f, 0), P(0.16f, 0.02f), P(0.16f, 0.20f), P(0.06f, 0.25f)},
        {P(0.06f, 0.25f), P(0.11f, 0.28f), P(0.11f, 0.38f), P(0.03f, 0.42f)},
        {P(0.03f, 0.42f), P(0.025f, 0.45f), P(0.03f, 0.48f), P(0.03f, 0.50f)},
        bezierLine(P(0.03f, 0.50f), P(0, 0.50f)),
    };
    Mesh mesh = revolve(p, 10, 48, m);
    mesh.name = "binh ho lo ma vang";
    return mesh;
}

Mesh bronzeJar(const Material& m) {
    Profile p;
    p.segments = {
        bezierLine(P(0, 0), P(0.15f, 0)),
        {P(0.15f, 0), P(0.15f, 0.12f), P(0.175f, 0.26f), P(0.165f, 0.38f)},
        bezierLine(P(0.165f, 0.38f), P(0.172f, 0.385f)),
        {P(0.172f, 0.385f), P(0.175f, 0.42f), P(0.10f, 0.45f), P(0.04f, 0.46f)},
        {P(0.04f, 0.46f), P(0.035f, 0.49f), P(0.045f, 0.50f), P(0, 0.505f)},
    };
    Mesh mesh = revolve(p, 8, 48, m);
    mesh.name = "thap dong Dao Thinh";
    return mesh;
}

Mesh jadeSeal(const Material& m) {
    Mesh mesh = makeBox({0.08f, 0.05f, 0.08f}, m);
    for (Vertex& v : mesh.vertices) v.position.y += 0.05f;
    append(mesh, makeCylinder(0.03f, 0.05f, 24, m), translate({0, 0.1f, 0}));
    append(mesh, makeSphere(0.045f, 12, 24, m), translate({0, 0.17f, 0}));
    mesh.name = "an ngoc";
    return mesh;
}

// Tu kinh: be (buc) + co vat + 2 lop kinh (mat ngoai + mat trong, de nhin xuyen thay ca kinh phia sau)
// + khung dong o 12 canh.
void addCase(Museum& mu, int zone, Vec3 pos, Vec3 baseHalf, Vec3 glassHalf, Mesh artifact, const char* text,
             const Material& baseMat, const Material& glassMat, const Material& trimMat) {
    SceneObject base = placed(makeBox(baseHalf, baseMat), translate(pos + Vec3{0, baseHalf.y, 0}));
    base.mesh.name = std::string("be tu ") + artifact.name;
    const int b = mu.add(std::move(base), zone, ObjKind::Stand, true, text);

    const float top = 2 * baseHalf.y;
    mu.add(placed(std::move(artifact), translate(pos + Vec3{0, top, 0})), zone, ObjKind::Exhibit, false, text, b);

    const Vec3 c = pos + Vec3{0, top + glassHalf.y, 0};
    const unsigned noBottom = 0x3Fu & ~(1u << 3);
    SceneObject outer = placed(makeBox(glassHalf, glassMat, false, noBottom), translate(c));
    SceneObject inner = placed(makeBox(glassHalf, glassMat, true, noBottom), translate(c));
    outer.mesh.name = inner.mesh.name = "kinh tu trung bay";
    mu.add(std::move(outer), zone, ObjKind::Glass, false, text, b);
    mu.add(std::move(inner), zone, ObjKind::Glass, false, text, b);

    Mesh trim;
    trim.name = "khung tu";
    trim.material = trimMat;
    const float e = 0.012f;
    const Vec3 g = glassHalf;
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            append(trim, makeBox({e, g.y, e}, trimMat), translate({sx * g.x, 0, sz * g.z}));
    for (int sy = -1; sy <= 1; sy += 2)
        for (int s = -1; s <= 1; s += 2) {
            append(trim, makeBox({g.x, e, e}, trimMat), translate({0, sy * g.y, s * g.z}));
            append(trim, makeBox({e, e, g.z}, trimMat), translate({s * g.x, sy * g.y, 0}));
        }
    mu.add(placed(std::move(trim), translate(c)), zone, ObjKind::Decor, false, text, b);

    const float pad = 0.05f;
    mu.grid.block({pos.x - baseHalf.x - pad, pos.z - baseHalf.z - pad, pos.x + baseHalf.x + pad, pos.z + baseHalf.z + pad});
}

void blockFootprint(Museum& mu, const SceneObject& o) {
    float x0 = 1e30f, z0 = 1e30f, x1 = -1e30f, z1 = -1e30f;
    for (const Vertex& v : o.mesh.vertices) {
        const Vec3 p = transformPoint(o.model, v.position);
        x0 = std::min(x0, p.x); x1 = std::max(x1, p.x);
        z0 = std::min(z0, p.z); z1 = std::max(z1, p.z);
    }
    mu.grid.block({x0 - 0.05f, z0 - 0.05f, x1 + 0.05f, z1 + 0.05f});
}

}  // namespace

// ============================================================================
// 6. DUNG CA BAO TANG
// ============================================================================
Museum buildMuseum(const std::string& objPath, std::string* objError) {
    Museum mu;
    // Sanh chinh: dung nguyen phong cu (vat lieu, 5 buc, buc giua nap .obj).
    std::vector<SceneObject> hall = buildRoom(objPath, objError);
    const Material woodFloor = hall[0].mesh.material;
    const Material hallWall = hall[1].mesh.material;
    const Material ceiling = hall[2].mesh.material;

    Material stoneFloor = mat("san da toi", {0.5f, 0.5f, 0.55f}, {0.35f, 0.35f, 0.35f}, 48);
    if (attach(stoneFloor, "assets/textures/da_cam_thach.png", mu.warnings)) {
        stoneFloor.kd = {0.42f, 0.42f, 0.46f};
        stoneFloor.ka = stoneFloor.kd * 0.15f;
    }

    // 4 khu va den rieng tung khu. Sanh giu 2 den cu (am ben trai, lanh ben phai).
    Material paving = mat("san lat da", {0.62f, 0.60f, 0.56f}, {0.1f, 0.1f, 0.1f}, 12);
    if (attach(paving, "assets/textures/da_cam_thach.png", mu.warnings)) {
        paving.kd = {0.70f, 0.66f, 0.58f};
        paving.ka = paving.kd * 0.3f;  // ngoai troi: anh sang troi (moi truong) manh hon trong nha
    }

    // Den tha tran, suy giam theo khoang cach (1 / (1 + k d^2)) nen moi den sang mot vung quanh no.
    auto lamp = [](float x, float y, float z, Vec3 c, float k) { return Light{{x, y, z}, c, k}; };
    const Vec3 warm{1.0f, 0.90f, 0.74f}, neutral{0.92f, 0.92f, 0.95f}, cool{0.80f, 0.86f, 1.0f};
    mu.zones = {
        {"sanh chinh", kHallArea, {lamp(-2.5f, 2.7f, 1.0f, warm * 1.05f, 0.07f), lamp(2.5f, 2.7f, -2.0f, cool * 0.95f, 0.07f), lamp(0, 2.8f, -3.0f, warm * 0.7f, 0.1f)}, woodFloor},
        {"hanh lang A", kCorridorA, {lamp(9, 2.85f, -3.25f, warm * 0.75f, 0.18f), lamp(12, 2.85f, -3.25f, warm * 0.75f, 0.18f), lamp(15, 2.85f, -3.25f, warm * 0.75f, 0.18f)}, woodFloor},
        {"hanh lang B", kCorridorB, {lamp(9, 2.85f, 3.25f, neutral * 0.75f, 0.18f), lamp(12, 2.85f, 3.25f, neutral * 0.75f, 0.18f), lamp(15, 2.85f, 3.25f, neutral * 0.75f, 0.18f)}, woodFloor},
        {"phong co vat", kArtifactRoom, {lamp(21, 2.9f, -3.0f, neutral * 0.8f, 0.08f), lamp(21, 2.9f, 3.0f, neutral * 0.8f, 0.08f), lamp(25.5f, 2.9f, 0, warm * 0.9f, 0.08f)}, stoneFloor},
        {"sanh don", kLobby, {lamp(0, 2.75f, 8.0f, warm * 0.95f, 0.12f)}, woodFloor},
        // Ngoai troi: "mat troi" o rat xa va cao, khong suy giam -> anh sang gan nhu song song.
        {"san truoc", kPorch, {Light{{8, 40, 30}, {0.95f, 0.92f, 0.84f}, 0}}, paving, true},
    };

    // Luoi 0,25 m phu ca mat bang.
    WalkGrid& g = mu.grid;
    g.cell = 0.25f;
    g.x0 = kHallArea.x0;
    g.z0 = kHallArea.z0;
    g.nx = static_cast<int>(std::lround((kArtifactRoom.x1 - g.x0) / g.cell));
    g.nz = static_cast<int>(std::lround((kPorch.z1 - g.z0) / g.cell));
    g.zone.assign(static_cast<std::size_t>(g.nx) * g.nz, -1);
    g.blocked.assign(g.zone.size(), 0);
    for (int j = 0; j < g.nz; ++j)
        for (int i = 0; i < g.nx; ++i) {
            const float x = g.x0 + (i + 0.5f) * g.cell, z = g.z0 + (j + 0.5f) * g.cell;
            for (int k = 0; k < static_cast<int>(mu.zones.size()); ++k)
                if (mu.zones[k].area.contains(x, z)) { g.zone[static_cast<std::size_t>(j) * g.nx + i] = k; break; }
        }

    // Giay dan tuong: 1 anh hoa van xam nhat (hoa_van_tuong.png) nhan voi mau kd rieng tung khu.
    ShellStyle style;
    style.walls = {
        mat("giay dan tuong kem", hallWall.kd, {0.03f, 0.03f, 0.03f}, 6),
        mat("giay dan tuong do tham", {0.52f, 0.15f, 0.13f}, {0.03f, 0.03f, 0.03f}, 6),
        mat("giay dan tuong xanh dam", {0.16f, 0.27f, 0.38f}, {0.03f, 0.03f, 0.03f}, 6),
        mat("giay dan tuong xam da", {0.26f, 0.27f, 0.32f}, {0.04f, 0.04f, 0.04f}, 6),
        mat("giay dan tuong kem", hallWall.kd, {0.03f, 0.03f, 0.03f}, 6),
    };
    for (Material& m : style.walls) attach(m, "assets/textures/hoa_van_tuong.png", mu.warnings);
    Material facade = mat("da op mat tien", {0.80f, 0.74f, 0.62f}, {0.08f, 0.08f, 0.08f}, 10);
    if (attach(facade, "assets/textures/da_cam_thach.png", mu.warnings)) {
        facade.kd = {0.86f, 0.78f, 0.64f};
        facade.ka = facade.kd * 0.3f;
    }
    style.walls.push_back(facade);
    style.ceiling = ceiling;
    style.dado = mat("op go oc cho", {0.48f, 0.32f, 0.20f}, {0.25f, 0.22f, 0.18f}, 32);
    attach(style.dado, "assets/textures/san_go.png", mu.warnings);
    style.trim = mat("nep go son trang", {0.90f, 0.88f, 0.82f}, {0.3f, 0.3f, 0.3f}, 40);
    style.portal = mat("go lim", {0.30f, 0.17f, 0.10f}, {0.35f, 0.3f, 0.25f}, 48);
    style.gilt = mat("thep vang", {0.80f, 0.60f, 0.22f}, {0.95f, 0.8f, 0.5f}, 90);
    buildShell(mu, style);

    // Sanh: cac cap (buc, vat) cua phong cu.
    for (std::size_t k = kRoomShellCount; k + 1 < hall.size(); k += 2) {
        blockFootprint(mu, hall[k]);
        const int stand = mu.add(std::move(hall[k]), 0, ObjKind::Stand, true);
        mu.add(std::move(hall[k + 1]), 0, ObjKind::Exhibit, true, "", stand);
    }

    // Hai hanh lang tranh, moi ben tuong 3 buc cach nhau 3 m.
    const Material gold = mat("khung go thep vang", {0.62f, 0.46f, 0.18f}, {0.7f, 0.6f, 0.35f}, 64);
    const PaintingDef paintings[] = {
        {"dam_cuoi_chuot.jpg", "Dam cuoi chuot (tranh Dong Ho)", "nghe nhan lang Dong Ho, Bac Ninh", 1, 9, true},
        {"phu_dong_thien_vuong.jpg", "Phu Dong Thien Vuong (tranh Dong Ho)", "nghe nhan lang Dong Ho, the ky 19", 1, 12, true},
        {"hai_ba_trung.jpg", "Hai Ba Trung (tranh Dong Ho)", "nghe nhan lang Dong Ho", 1, 15, true},
        {"lon_am_duong.jpg", "Lon am duong (tranh Dong Ho)", "nghe nhan lang Dong Ho", 1, 9, false},
        {"muc_dong_doc_sach.jpg", "Muc dong doc sach (tranh Dong Ho)", "nghe nhan lang Dong Ho", 1, 12, false},
        {"song_lon_kanagawa.jpg", "Song lon ngoai khoi Kanagawa", "Katsushika Hokusai, khoang 1831", 1, 15, false},
        {"mona_lisa.jpg", "Mona Lisa", "Leonardo da Vinci, 1503-1506", 2, 9, true},
        {"thieu_nu_ngoc_trai.jpg", "Thieu nu deo hoa tai ngoc trai", "Johannes Vermeer, khoang 1665", 2, 12, true},
        {"dem_day_sao.jpg", "Dem day sao", "Vincent van Gogh, 1889", 2, 15, true},
        {"an_tuong_mat_troi_moc.jpg", "An tuong, mat troi moc", "Claude Monet, 1872", 2, 9, false},
        {"nu_hon.jpg", "Nu hon", "Gustav Klimt, 1907-1908", 2, 12, false},
        {"tho_san_tren_tuyet.jpg", "Tho san tren tuyet", "Pieter Bruegel cha, 1565", 2, 15, false},
    };
    for (const PaintingDef& d : paintings) addPainting(mu, d, gold);

    // Phong co vat: 7 tu kinh — 1 tu lon o giua, 3 tu doc moi tuong dai.
    Material glass = mat("kinh", {0.05f, 0.07f, 0.07f}, {0.9f, 0.9f, 0.9f}, 180);
    glass.ka = {0.015f, 0.02f, 0.02f};
    glass.opacity = 0.12f;
    const Material wood = mat("go mun", {0.20f, 0.12f, 0.08f}, {0.25f, 0.2f, 0.15f}, 32);
    const Material brass = mat("dong thau", {0.65f, 0.5f, 0.25f}, {0.8f, 0.7f, 0.45f}, 96);

    Material bronze = mat("dong co", {0.95f, 0.95f, 0.95f}, {0.55f, 0.48f, 0.32f}, 40);
    if (!attach(bronze, "assets/textures/dong_son.png", mu.warnings)) bronze.kd = {0.42f, 0.36f, 0.22f};
    bronze.ka = bronze.kd * 0.15f;
    Material porcelain = mat("gom hoa lam", {0.95f, 0.95f, 0.95f}, {0.7f, 0.7f, 0.7f}, 110);
    if (!attach(porcelain, "assets/textures/gom_hoa_lam.png", mu.warnings)) porcelain.kd = {0.85f, 0.88f, 0.95f};
    porcelain.ka = porcelain.kd * 0.15f;
    const Material burner = mat("dong vang", {0.62f, 0.45f, 0.2f}, {0.85f, 0.7f, 0.4f}, 90);
    const Material jade = mat("ngoc bich", {0.22f, 0.55f, 0.38f}, {0.6f, 0.7f, 0.65f}, 100);
    const Material goldMat = mat("ma vang", {0.85f, 0.62f, 0.2f}, {0.95f, 0.8f, 0.5f}, 120);
    const Material whiteJade = mat("ngoc trang", {0.86f, 0.88f, 0.8f}, {0.5f, 0.5f, 0.5f}, 80);

    const int room = 3;
    const Vec3 baseHalf{0.4f, 0.45f, 0.4f}, glassHalf{0.4f, 0.42f, 0.4f};
    addCase(mu, room, {23, 0, 0}, {0.6f, 0.3f, 0.6f}, {0.6f, 0.42f, 0.6f}, bronzeDrum(bronze),
            "Trong dong Ngoc Lu (mo phong) - van hoa Dong Son, cach nay khoang 2500 nam. Mat trong: ngoi sao 14 canh",
            wood, glass, brass);
    addCase(mu, room, {21, 0, -5}, baseHalf, glassHalf, blueWhiteVase(porcelain),
            "Binh gom hoa lam (mo phong) - gom Chu Dau, the ky 15", wood, glass, brass);
    addCase(mu, room, {23.5f, 0, -5}, baseHalf, glassHalf, incenseBurner(burner),
            "Lu huong dong ba chan (mo phong) - thoi Le", wood, glass, brass);
    addCase(mu, room, {26, 0, -5}, baseHalf, glassHalf, jadeCup(jade),
            "Chen va dia ngoc bich (mo phong)", wood, glass, brass);
    addCase(mu, room, {21, 0, 5}, baseHalf, glassHalf, goldGourd(goldMat),
            "Binh ho lo ma vang (mo phong) - thoi Nguyen", wood, glass, brass);
    addCase(mu, room, {23.5f, 0, 5}, baseHalf, glassHalf, bronzeJar(bronze),
            "Thap dong Dao Thinh (mo phong) - van hoa Dong Son", wood, glass, brass);
    addCase(mu, room, {26, 0, 5}, baseHalf, glassHalf, jadeSeal(whiteJade),
            "An ngoc (mo phong) - an trieu Nguyen", wood, glass, brass);

    // Cua chinh (z = 10, giua sanh don va san truoc): khung cua tu sinh (2 cot + dam), 2 canh go lim
    // MO SAN vao trong, ap sat 2 tuong ben cua sanh don (ban le o 2 cot). Canh cua la vat can mong.
    const int lobby = 4, porch = 5;
    const float doorZ = kLobby.z1;
    Mesh doorWood, doorBrass;
    doorWood.name = "canh cua chinh (dang mo)";
    doorWood.material = style.portal;
    doorBrass.name = "tay nam cua";
    doorBrass.material = style.gilt;
    const Vec3 zAxis{0, 0, 1};
    for (int s = -1; s <= 1; s += 2) {
        const float x = s * 1.93f, cz = doorZ - 0.2f - 0.9f;   // canh rong 1,8 m, nam doc tuong ben
        bar(doorWood, {x, 1.25f, cz}, zAxis, 0.9f, 1.25f, 0.035f, style.portal);
        bar(doorWood, {x - s * 0.045f, 0.7f, cz}, zAxis, 0.7f, 0.42f, 0.012f, style.portal);   // o noi duoi
        bar(doorWood, {x - s * 0.045f, 1.8f, cz}, zAxis, 0.7f, 0.5f, 0.012f, style.portal);    // o noi tren
        bar(doorBrass, {x - s * 0.07f, 1.1f, cz - 0.75f}, Vec3{1, 0, 0}, 0.018f, 0.16f, 0.03f, style.gilt);  // tay nam
        mu.grid.block({std::min(x, s * 2.0f) - 0.05f, cz - 0.9f, std::max(x, s * 2.0f) + 0.05f, cz + 0.9f});
    }
    mu.add(placed(std::move(doorWood), Mat4::identity()), lobby, ObjKind::Decor, false, "Cua chinh vao bao tang (luon mo)");
    mu.add(placed(std::move(doorBrass), Mat4::identity()), lobby, ObjKind::Decor, false, "Cua chinh vao bao tang (luon mo)");

    Material terracotta = mat("chau dat nung", {0.66f, 0.34f, 0.20f}, {0.15f, 0.12f, 0.1f}, 16);
    const Material leaves = mat("la cay", {0.18f, 0.42f, 0.16f}, {0.2f, 0.25f, 0.2f}, 20);
    Profile pot;
    pot.segments = {
        bezierLine(P(0, 0), P(0.14f, 0)),
        {P(0.14f, 0), P(0.16f, 0.15f), P(0.20f, 0.30f), P(0.21f, 0.40f)},
        bezierLine(P(0.21f, 0.40f), P(0.19f, 0.40f)),
        bezierLine(P(0.19f, 0.40f), P(0.18f, 0.36f)),
        bezierLine(P(0.18f, 0.36f), P(0, 0.36f)),
    };
    const struct { Vec3 at; int zone; } plants[] = {
        {{-1.45f, 0, 6.9f}, lobby}, {{1.45f, 0, 6.9f}, lobby},
        {{-4.3f, 0, 10.8f}, porch}, {{4.3f, 0, 10.8f}, porch}, {{-4.3f, 0, 14.2f}, porch}, {{4.3f, 0, 14.2f}, porch},
    };
    for (const auto& pl : plants) {
        const Vec3 at = pl.at;
        Mesh bush = makeSphere(0.28f, 10, 16, leaves);
        for (Vertex& v : bush.vertices) v.position.y += 0.66f;
        append(bush, makeSphere(0.2f, 8, 14, leaves), translate({0.14f, 0.86f, 0.05f}));
        append(bush, makeSphere(0.22f, 8, 14, leaves), translate({-0.12f, 0.9f, -0.08f}));
        append(bush, makeSphere(0.16f, 8, 12, leaves), translate({0.02f, 1.08f, 0}));
        bush.name = "cay canh";
        const int p = mu.add(placed(revolve(pot, 6, 28, terracotta), translate(at)), pl.zone, ObjKind::Decor, true, "Chau cay canh");
        mu.objects[p].mesh.name = "chau cay";
        mu.add(placed(std::move(bush), translate(at)), pl.zone, ObjKind::Decor, true, "Chau cay canh");
        mu.grid.block({at.x - 0.3f, at.z - 0.3f, at.x + 0.3f, at.z + 0.3f});
    }
    return mu;
}
