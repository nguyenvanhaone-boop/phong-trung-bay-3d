// PHAN B — CANH VA CAMERA (Hao).
#include "scene.hpp"

#include <algorithm>
#include <cmath>

#include "modeling/modeling.hpp"

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

std::vector<SceneObject> buildRoom() {
    const Material floorMat = mat("san go", {0.55f, 0.38f, 0.24f}, {0.15f, 0.15f, 0.15f}, 16);
    const Material wallMat = mat("tuong", {0.85f, 0.83f, 0.78f}, {0.02f, 0.02f, 0.02f}, 4);
    const Material ceilMat = mat("tran", {0.92f, 0.92f, 0.92f}, {0, 0, 0}, 1);
    const Material standMat = mat("buc da", {0.62f, 0.62f, 0.66f}, {0.2f, 0.2f, 0.2f}, 24);

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

        // Vat trung bay: dung mat tron xoay cua phan A (Khang). Khi phan A chua cai xong
        // (makeSphere tra ve luoi rong), tam dat mot hop nho xoay 45 do.
        Mesh exhibit = makeSphere(0.25f, 24, 32, exhibitMats[i]);
        float yaw = 0, lift = 0.25f;  // lift = khoang cach tu mat buc den tam vat
        if (exhibit.triangles.empty()) {
            exhibit = makeBox({0.18f, 0.18f, 0.18f}, exhibitMats[i]);
            exhibit.name = "vat tam (cho phan A)";
            yaw = 45;
            lift = 0.18f;
        }
        room.push_back(place(std::move(exhibit), {base.x, 2 * standHalf.y + lift, base.z}, yaw));
    }
    return room;
}

// ============================================================================
// 6. XEN MAT GAN — Sprint 3 (DHMTPTB3-30)
// ============================================================================
std::vector<ViewVertex> clipNear(const ViewVertex tri[3], float nearPlane) {
    // TODO(Hao, Sprint 3): Sutherland–Hodgman voi mot mat phang z = -near;
    // canh cat mat phang -> noi suy moi thuoc tinh (view, world, normal, u, v) theo cung tham so.
    // Tam thoi: bo han tam giac co dinh nam sau mat phang gan (tranh chia cho w <= 0).
    for (int i = 0; i < 3; ++i)
        if (tri[i].view.z > -nearPlane) return {};
    return {tri[0], tri[1], tri[2]};
}
