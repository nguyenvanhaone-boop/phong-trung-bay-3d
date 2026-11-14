// PHAN B — CANH VA CAMERA (Hao).
#include "scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "modeling/modeling.hpp"
#include "obj_loader.hpp"
#include "texture_loader.hpp"

// ============================================================================
// 1. MA TRAN QUAN SAT (look-at) — slide CG.4 "He quan sat"
// ============================================================================
// Muc dich: doi toa do the gioi sang he toa do gan voi mat (camera):
//   goc toa do = vi tri mat E, truc -z = huong nhin, truc y = huong len, truc x = ben phai.
//
// Ba truc cua he quan sat (deu don vi, vuong goc nhau):
//   f = normalize(C - E)        huong nhin (forward)
//   s = normalize(f x up)       ben phai (side)
//   u = s x f                   huong len that su (up), vuong goc voi f va s
//
// Ma tran = [quay] x [tinh tien -E]:
//   | s.x   s.y   s.z   -s.E |
//   | u.x   u.y   u.z   -u.E |
//   | -f.x  -f.y  -f.z   f.E |
//   | 0     0     0      1   |
// Hang 1-3 la 3 truc (quay the gioi cho truc trung voi truc camera);
// cot 4 dua mat E ve goc toa do. Kiem tra: lookAt * E = (0, 0, 0).
Mat4 lookAt(Vec3 E, Vec3 C, Vec3 up) {
    const Vec3 f = normalize(C - E);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 m = Mat4::identity();
    m.m[0][0] = s.x;  m.m[0][1] = s.y;  m.m[0][2] = s.z;  m.m[0][3] = -dot(s, E);
    m.m[1][0] = u.x;  m.m[1][1] = u.y;  m.m[1][2] = u.z;  m.m[1][3] = -dot(u, E);
    m.m[2][0] = -f.x; m.m[2][1] = -f.y; m.m[2][2] = -f.z; m.m[2][3] = dot(f, E);
    return m;
}

// Huong nhin tu 2 goc (toa do cau):
//   di bo:  f = (cos(pitch) sin(yaw), sin(pitch), -cos(pitch) cos(yaw))   -> yaw = pitch = 0 nhin theo -z
//   orbit:  mat E = target + R (cos(phi) sin(theta), sin(phi), cos(phi) cos(theta)), f = target - E
Vec3 Camera::eyePosition() const {
    if (mode == Mode::Walk) return eye;
    const Vec3 dir{std::cos(orbitPhi) * std::sin(orbitTheta), std::sin(orbitPhi), std::cos(orbitPhi) * std::cos(orbitTheta)};
    return target + dir * orbitRadius;
}

Vec3 Camera::forward() const {
    if (mode == Mode::Walk)
        return {std::cos(pitch) * std::sin(yaw), std::sin(pitch), -std::cos(pitch) * std::cos(yaw)};
    return normalize(target - eyePosition());
}

Mat4 Camera::viewMatrix() const {
    const Vec3 E = eyePosition();
    return lookAt(E, E + forward(), Vec3{0, 1, 0});
}

// ============================================================================
// 2. PHEP CHIEU — slide CG.4 "Phep chieu"
// ============================================================================
// Ca hai phep chieu dua hop nhin ve khoi lap phuong chuan [-1, 1]^3 (NDC).
// Renderer chia cho w roi doi NDC sang pixel.
//
// Phoi canh (tam chieu o mat): diem cang xa cang nho vi chia cho w = -z.
//   t = 1 / tan(fovy / 2)
//   | t/aspect  0   0                  0                 |
//   | 0         t   0                  0                 |
//   | 0         0   (f+n)/(n-f)        2fn/(n-f)         |
//   | 0         0   -1                 0                 |
//
// Song song (truc giao): tia chieu song song voi truc nhin, khong chia, w = 1.
//   Hop nhin [-h*aspect, h*aspect] x [-h, h] x [-n, -f]
//   | 1/(h*aspect)  0     0          0             |
//   | 0             1/h   0          0             |
//   | 0             0     -2/(f-n)   -(f+n)/(f-n)  |
//   | 0             0     0          1             |
//   h chon bang nua chieu cao ma phoi canh nhin thay o khoang cach "d" (ban kinh orbit,
//   hoac 3 m khi di bo), nen khi bam P vat khong dot ngot to hay nho di.
Mat4 Camera::projectionMatrix(float aspect) const {
    const float n = nearPlane, f = farPlane;
    const float halfFov = radians(fovyDeg) * 0.5f;
    Mat4 m;
    if (projection == Projection::Perspective) {
        const float t = 1.0f / std::tan(halfFov);
        m.m[0][0] = t / aspect;
        m.m[1][1] = t;
        m.m[2][2] = (f + n) / (n - f);
        m.m[2][3] = 2 * f * n / (n - f);
        m.m[3][2] = -1;
    } else {
        const float d = (mode == Mode::Orbit) ? orbitRadius : 3.0f;
        const float h = d * std::tan(halfFov);
        m.m[0][0] = 1.0f / (h * aspect);
        m.m[1][1] = 1.0f / h;
        m.m[2][2] = -2.0f / (f - n);
        m.m[2][3] = -(f + n) / (f - n);
        m.m[3][3] = 1;
    }
    return m;
}

// ============================================================================
// 3. DIEU KHIEN CAMERA
// ============================================================================
void Camera::walk(float forwardStep, float rightStep) {
    if (mode != Mode::Walk) return;
    // Di tren mat san: bo thanh phan y cua huong nhin, de nhin len/xuong khong lam "bay".
    const Vec3 ahead{std::sin(yaw), 0, -std::cos(yaw)};
    const Vec3 right{std::cos(yaw), 0, std::sin(yaw)};   // = ahead x (0,1,0)
    eye = eye + ahead * forwardStep + right * rightStep;
    // Kep trong phong (va cham don gian voi 4 buc tuong).
    eye.x = std::clamp(eye.x, -kRoomHalfX + kWallMargin, kRoomHalfX - kWallMargin);
    eye.z = std::clamp(eye.z, -kRoomHalfZ + kWallMargin, kRoomHalfZ - kWallMargin);
    eye.y = kEyeHeight;
}

void Camera::rotate(float dYaw, float dPitch) {
    // Gioi han +-89 do: o +-90 do huong nhin song song voi up, f x up = 0, ma tran look-at hong.
    const float limit = radians(89.0f);
    if (mode == Mode::Walk) {
        yaw = std::remainder(yaw + dYaw, 2 * kPi);  // giu yaw trong [-180, 180] do (quay 1 vong = nhu cu)
        pitch = std::clamp(pitch + dPitch, -limit, limit);
    } else {
        orbitTheta -= dYaw;
        orbitPhi = std::clamp(orbitPhi + dPitch, -limit, limit);
    }
}

void Camera::zoom(float factor) {
    orbitRadius = std::clamp(orbitRadius * factor, 0.8f, 15.0f);
}

// ============================================================================
// 4. HINH HOP — khoi xay dung cua san, tuong, buc
// ============================================================================
// Moi mat la mot hinh vuong 4 dinh. Voi mat co phap tuyen n, chon truc a nam trong mat
// va b = n x a, khi do a x b = n nen 4 goc (-,-) (+,-) (+,+) (-,+) theo truc (a, b)
// xep NGUOC chieu kim dong ho khi nhin tu phia n (dung quy uoc trong mesh.hpp).
Mesh makeBox(Vec3 half, const Material& material, bool inward, unsigned faces) {
    struct FaceDef { Vec3 n, a; };
    const FaceDef defs[6] = {
        {{1, 0, 0}, {0, 0, -1}},  // +x
        {{-1, 0, 0}, {0, 0, 1}},  // -x
        {{0, 1, 0}, {1, 0, 0}},   // +y (mat tren)
        {{0, -1, 0}, {1, 0, 0}},  // -y (mat duoi)
        {{0, 0, 1}, {1, 0, 0}},   // +z
        {{0, 0, -1}, {-1, 0, 0}}, // -z
    };
    auto extent = [&](Vec3 axis) {  // nua kich thuoc cua hop theo mot truc toa do
        return std::fabs(axis.x) * half.x + std::fabs(axis.y) * half.y + std::fabs(axis.z) * half.z;
    };

    Mesh m;
    m.name = inward ? "hop (mat trong)" : "hop";
    m.material = material;
    for (int i = 0; i < 6; ++i) {
        if (!(faces & (1u << i))) continue;
        const Vec3 n = defs[i].n, a = defs[i].a, b = cross(n, a);
        const Vec3 c = n * extent(n);
        const float ea = extent(a), eb = extent(b);
        const Vec3 normal = inward ? -n : n;
        const int base = static_cast<int>(m.vertices.size());
        const float su[4] = {-1, 1, 1, -1}, sv[4] = {-1, -1, 1, 1};
        for (int k = 0; k < 4; ++k) {
            Vertex v;
            v.position = c + a * (su[k] * ea) + b * (sv[k] * eb);
            v.normal = normal;
            v.u = (su[k] + 1) * 0.5f * ea;  // toa do texture theo met: anh se lap lai 1 lan / 1 m
            v.v = (sv[k] + 1) * 0.5f * eb;
            m.vertices.push_back(v);
        }
        if (!inward) {
            m.triangles.push_back({base, base + 1, base + 2});
            m.triangles.push_back({base, base + 2, base + 3});
        } else {  // dao chieu: nhin tu ben trong moi la nguoc chieu kim dong ho
            m.triangles.push_back({base, base + 2, base + 1});
            m.triangles.push_back({base, base + 3, base + 2});
        }
    }
    return m;
}

// ============================================================================
// 5. PHONG TRUNG BAY
// ============================================================================
namespace {

// Gan texture cho vat lieu (DHMTPTB3-41). Anh nhan voi kd, nen khi co anh dat kd gan trang
// de mau lay tu anh; khong nap duoc anh thi giu mau kd cu (canh van dung, chi khong co van).
void attachTexture(Material& m, const char* path) {
    std::string err;
    std::shared_ptr<const Texture> tex = loadTexture(path, err);
    if (!tex) {
        std::fprintf(stderr, "Khong nap duoc texture %s\n", err.c_str());
        return;
    }
    m.texture = std::move(tex);
    m.kd = {0.95f, 0.95f, 0.95f};
    m.ka = m.kd * 0.15f;
}

Material mat(const char* name, Vec3 kd, Vec3 ks, float shininess) {
    Material m;
    m.name = name;
    m.ka = kd * 0.15f;
    m.kd = kd;
    m.ks = ks;
    m.shininess = shininess;
    return m;
}

SceneObject place(Mesh mesh, Vec3 position, float yawDeg = 0) {
    SceneObject o;
    o.mesh = std::move(mesh);
    o.model = translate(position) * rotateY(radians(yawDeg));  // chi tinh tien + quay: giu phap tuyen dung
    return o;
}

}  // namespace

std::vector<SceneObject> buildRoom(const std::string& objPath, std::string* objError) {
    Material floorMat = mat("san go", {0.55f, 0.38f, 0.24f}, {0.15f, 0.15f, 0.15f}, 16);
    attachTexture(floorMat, "assets/textures/san_go.png");
    const Material wallMat = mat("tuong", {0.85f, 0.83f, 0.78f}, {0.02f, 0.02f, 0.02f}, 4);
    const Material ceilMat = mat("tran", {0.92f, 0.92f, 0.92f}, {0, 0, 0}, 1);
    Material standMat = mat("buc da", {0.62f, 0.62f, 0.66f}, {0.2f, 0.2f, 0.2f}, 24);
    attachTexture(standMat, "assets/textures/da_cam_thach.png");

    // Vat lieu cho vat trung bay (xem bang vat lieu trong tai lieu nghien cuu).
    const Material exhibitMats[5] = {
        mat("gom trang men", {0.92f, 0.90f, 0.85f}, {0.5f, 0.5f, 0.5f}, 64),
        mat("dong", {0.55f, 0.35f, 0.15f}, {0.8f, 0.6f, 0.4f}, 128),
        mat("nhua do", {0.80f, 0.10f, 0.10f}, {0.3f, 0.3f, 0.3f}, 32),
        mat("gom xanh", {0.20f, 0.40f, 0.65f}, {0.5f, 0.5f, 0.5f}, 64),
        mat("dat nung", {0.70f, 0.40f, 0.25f}, {0.05f, 0.05f, 0.05f}, 8),
    };

    std::vector<SceneObject> room;

    // San, tuong, tran: mot hop lon nhin tu ben trong, tach 3 phan de gan 3 vat lieu.
    const Vec3 roomHalf{kRoomHalfX, kRoomHeight * 0.5f, kRoomHalfZ};
    const Vec3 roomCenter{0, kRoomHeight * 0.5f, 0};
    const unsigned walls = (1u << 0) | (1u << 1) | (1u << 4) | (1u << 5);
    room.push_back(place(makeBox(roomHalf, floorMat, true, 1u << 3), roomCenter));  // mat -y = san
    room.push_back(place(makeBox(roomHalf, wallMat, true, walls), roomCenter));
    room.push_back(place(makeBox(roomHalf, ceilMat, true, 1u << 2), roomCenter));   // mat +y = tran
    room[0].mesh.name = "san";
    room[1].mesh.name = "tuong";
    room[2].mesh.name = "tran";

    // 5 buc xep hinh chu U, nguoi xem vao tu phia +z.
    const Vec3 standHalf{0.35f, 0.5f, 0.35f};
    const Vec3 spots[5] = {{-3.5f, 0, 0.5f}, {-3.5f, 0, -3.0f}, {0, 0, -4.2f}, {3.5f, 0, -3.0f}, {3.5f, 0, 0.5f}};
    for (int i = 0; i < 5; ++i) {
        const Vec3 base = spots[i];
        SceneObject stand = place(makeBox(standHalf, standMat), {base.x, standHalf.y, base.z});
        stand.mesh.name = "buc " + std::to_string(i + 1);
        room.push_back(std::move(stand));

        // Vat trung bay: mat tron xoay tu profile Bezier cua phan A. Day vat dat tren mat buc.
        // Vat so 2 la qua cau (tam o goc) nen nang len 1 ban kinh.
        static const char* const names[5] = {"binh co thon", "binh co cao", "qua cau", "bat rong mieng", "lo mieng hep"};
        Mesh exhibit;
        float lift = 0;
        const float yaw = 0;
        if (i == 2) {
            exhibit = makeSphere(0.2f, 24, 32, exhibitMats[i]);
            lift = 0.2f;
        } else {
            exhibit = revolve(exhibitProfile(i), 8, 40, exhibitMats[i]);
        }
        exhibit.name = names[i];
        room.push_back(place(std::move(exhibit), {base.x, 2 * standHalf.y + lift, base.z}, yaw));
    }

    // Buc thu 6 o giua phong cho vat nap tu file .obj (DHMTPTB3-42), mac dinh la am tra Utah.
    if (!objPath.empty()) {
        Mesh imported;
        std::string err;
        if (loadObj(objPath, imported, err)) {
            fitToHeight(imported, 0.45f);  // doi toa do dinh, khong dung ma tran ti le
            imported.material = mat("su bac", {0.75f, 0.75f, 0.78f}, {0.9f, 0.9f, 0.9f}, 96);
            const Vec3 centerHalf{0.45f, 0.4f, 0.45f};
            const Vec3 spot{0, 0, -1.2f};
            SceneObject stand = place(makeBox(centerHalf, standMat), {spot.x, centerHalf.y, spot.z});
            stand.mesh.name = "buc giua";
            room.push_back(std::move(stand));
            room.push_back(place(std::move(imported), {spot.x, 2 * centerHalf.y, spot.z}, -30));
        } else if (objError) {
            *objError = err;
        }
    }
    return room;
}

// ============================================================================
// 6. CHON VAT BANG TIA — Sprint 4 (DHMTPTB3-36)
// ============================================================================
// Ma tran nghich dao bang Gauss–Jordan: bien doi [M | I] thanh [I | M^-1].
// Moi cot chon hang co |phan tu| lon nhat lam truc (pivot) de giam sai so.
bool inverse(const Mat4& m, Mat4& out) {
    float a[4][8];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) { a[i][j] = m.m[i][j]; a[i][j + 4] = (i == j) ? 1.0f : 0.0f; }
    for (int col = 0; col < 4; ++col) {
        int piv = col;
        for (int r = col + 1; r < 4; ++r)
            if (std::fabs(a[r][col]) > std::fabs(a[piv][col])) piv = r;
        if (std::fabs(a[piv][col]) < 1e-12f) return false;  // suy bien
        if (piv != col)
            for (int j = 0; j < 8; ++j) std::swap(a[piv][j], a[col][j]);
        const float inv = 1.0f / a[col][col];
        for (int j = 0; j < 8; ++j) a[col][j] *= inv;
        for (int r = 0; r < 4; ++r) {
            if (r == col || a[r][col] == 0) continue;
            const float f = a[r][col];
            for (int j = 0; j < 8; ++j) a[r][j] -= f * a[col][j];
        }
    }
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) out.m[i][j] = a[i][j + 4];
    return true;
}

// Pixel -> tia: lam NGUOC duong ong ve hinh.
//   pixel -> NDC: x = 2 px / w - 1, y = 1 - 2 py / h (y man hinh huong xuong)
//   Lay 2 diem cung (x, y): tren mat phang gan (z_ndc = -1) va mat phang xa (z_ndc = +1),
//   nhan voi (P * V)^-1 roi chia w -> 2 diem trong the gioi. Tia = tu diem gan toi diem xa.
// Cach nay dung cho ca phoi canh (cac tia toa ra tu mat) va song song (cac tia song song).
Ray rayFromPixel(const Camera& camera, float px, float py, int w, int h, float aspect) {
    const float x = 2.0f * px / w - 1.0f;
    const float y = 1.0f - 2.0f * py / h;
    Mat4 inv;
    if (!inverse(camera.projectionMatrix(aspect) * camera.viewMatrix(), inv))
        return {camera.eyePosition(), camera.forward()};
    auto unproject = [&](float zNdc) {
        const Vec4 p = inv * Vec4{x, y, zNdc, 1};
        return Vec3{p.x / p.w, p.y / p.w, p.z / p.w};
    };
    const Vec3 a = unproject(-1), b = unproject(1);
    return {a, normalize(b - a)};
}

namespace {

// Moller–Trumbore: giai O + t D = (1 - u - v) A + u B + v C.
// det > 0 khi tia nhin vao mat truoc (tam giac nguoc chieu kim dong ho khi nhin tu phia tia).
bool intersectTriangle(Vec3 O, Vec3 D, Vec3 A, Vec3 B, Vec3 C, float& t) {
    const Vec3 e1 = B - A, e2 = C - A;
    const Vec3 p = cross(D, e2);
    const float det = dot(e1, p);
    if (det < 1e-9f) return false;                 // mat quay lung hoac tia song song voi mat
    const float invDet = 1.0f / det;
    const Vec3 s = O - A;
    const float u = dot(s, p) * invDet;
    if (u < 0 || u > 1) return false;
    const Vec3 q = cross(s, e1);
    const float v = dot(D, q) * invDet;
    if (v < 0 || u + v > 1) return false;
    t = dot(e2, q) * invDet;
    return t > 0;
}

}  // namespace

// Giao trong he toa do CUA VAT (khong bien doi tung dinh sang the gioi):
//   dua tia ve he vat bang M^-1: O' = M^-1 O (w = 1), D' = M^-1 D (w = 0).
// KHONG chuan hoa D': khi do diem trung O' + t D' ung voi O + t D trong the gioi,
// nen t cua cac vat khac nhau so sanh truc tiep duoc (M la phep affine).
PickResult pick(const std::vector<SceneObject>& objects, const Ray& ray) {
    PickResult best;
    float bestT = 1e30f;
    for (std::size_t i = 0; i < objects.size(); ++i) {
        const SceneObject& obj = objects[i];
        if (!obj.visible) continue;
        Mat4 inv;
        if (!inverse(obj.model, inv)) continue;
        const Vec3 O = transformPoint(inv, ray.origin);
        const Vec3 D = transformDir(inv, ray.dir);
        const auto& vs = obj.mesh.vertices;
        for (const auto& tri : obj.mesh.triangles) {
            float t;
            if (intersectTriangle(O, D, vs[tri[0]].position, vs[tri[1]].position, vs[tri[2]].position, t) && t < bestT) {
                bestT = t;
                best.index = static_cast<int>(i);
            }
        }
    }
    if (best.index >= 0) {
        best.t = bestT;
        best.point = ray.origin + ray.dir * bestT;
    }
    return best;
}

void worldBounds(const SceneObject& object, Vec3& center, float& radius) {
    if (object.mesh.vertices.empty()) {
        center = transformPoint(object.model, {0, 0, 0});
        radius = 0;
        return;
    }
    Vec3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};
    for (const Vertex& v : object.mesh.vertices) {
        const Vec3 p = transformPoint(object.model, v.position);
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }
    center = (lo + hi) * 0.5f;
    radius = length(hi - lo) * 0.5f;
}

// ============================================================================
// 7. XEN MAT GAN — Sprint 3 (DHMTPTB3-30), slide CG.4 "Xen hinh"
// ============================================================================
// Vi sao phai xen: dinh nam sau mat (z >= 0) co w = -z <= 0, chia cho w lam dinh "lat" sang
// phia doi dien man hinh -> tam giac vo, keo dai ca man hinh (dung loi thay khi di sat vat).
// Xen trong he quan sat, TRUOC khi chieu, vi o day mat phang gan chi la z = -n.
//
// Sutherland–Hodgman voi MOT mat phang (slide CG.4): duyet tung canh (P -> Q) cua da giac,
//   "trong" = phia nhin thay: z <= -n.
//   P trong, Q trong : xuat Q
//   P trong, Q ngoai : xuat giao diem I
//   P ngoai, Q trong : xuat I roi Q
//   P ngoai, Q ngoai : khong xuat gi
// Giao diem tren canh P + t (Q - P), chon t sao cho z = -n:
//   t = (-n - P.z) / (Q.z - P.z)
// Moi thuoc tinh (view, world, normal, u, v) noi suy CUNG tham so t. Noi suy tuyen tinh o day
// la dung vi he quan sat chua qua phep chia phoi canh.
//
// Ket qua: 0 dinh (ca tam giac sau mat phang), 3 dinh (1 hoac 3 dinh trong), 4 dinh (2 dinh trong).
namespace {

ViewVertex lerpVertex(const ViewVertex& p, const ViewVertex& q, float t) {
    ViewVertex r;
    r.view = p.view + (q.view - p.view) * t;
    r.world = p.world + (q.world - p.world) * t;
    r.normal = normalize(p.normal + (q.normal - p.normal) * t);  // noi suy xong phai chuan hoa lai
    r.u = p.u + (q.u - p.u) * t;
    r.v = p.v + (q.v - p.v) * t;
    return r;
}

}  // namespace

std::vector<ViewVertex> clipNear(const ViewVertex tri[3], float nearPlane) {
    const float zn = -nearPlane;
    auto inside = [zn](const ViewVertex& v) { return v.view.z <= zn; };

    std::vector<ViewVertex> out;
    out.reserve(4);
    for (int i = 0; i < 3; ++i) {
        const ViewVertex& p = tri[i];
        const ViewVertex& q = tri[(i + 1) % 3];
        const bool pIn = inside(p), qIn = inside(q);
        if (pIn != qIn) {  // canh cat mat phang (hai dau khac phia nen Q.z != P.z)
            const float t = (zn - p.view.z) / (q.view.z - p.view.z);
            ViewVertex cut = lerpVertex(p, q, t);
            cut.view.z = zn;  // gan dung bang -n, tranh sai so lam dinh lot ra sau mat phang
            out.push_back(cut);
        }
        if (qIn) out.push_back(q);
    }
    return out;
}
