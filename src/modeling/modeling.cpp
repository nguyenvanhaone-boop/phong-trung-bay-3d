// PHAN A — MO HINH HOA (Khang). Slide CG.5 (duong cong Bezier), Lab 05.
#include "modeling.hpp"

#include <algorithm>
#include <cmath>

// ============================================================================
// 1. BEZIER BAC 3 — de Casteljau (slide CG.5)
// ============================================================================
// Noi suy tuyen tinh lap 3 lan tren 4 diem dieu khien:
//   muc 1: Q_i = (1 - t) P_i + t P_{i+1}       (3 diem)
//   muc 2: R_i = (1 - t) Q_i + t Q_{i+1}       (2 diem)
//   muc 3: B(t) = (1 - t) R_0 + t R_1          (1 diem, nam tren duong cong)
// Ket qua bang dang Bernstein: B(t) = (1-t)^3 P0 + 3t(1-t)^2 P1 + 3t^2(1-t) P2 + t^3 P3,
// nhung de Casteljau on dinh so hon va cho luon cac diem trung gian de ve minh hoa.
namespace {
Vec3 lerp(Vec3 a, Vec3 b, float t) { return a * (1 - t) + b * t; }
}  // namespace

std::vector<Vec3> deCasteljauLevels(const std::array<Vec3, 4>& P, float t) {
    const Vec3 q0 = lerp(P[0], P[1], t), q1 = lerp(P[1], P[2], t), q2 = lerp(P[2], P[3], t);
    const Vec3 r0 = lerp(q0, q1, t), r1 = lerp(q1, q2, t);
    return {q0, q1, q2, r0, r1, lerp(r0, r1, t)};
}

Vec3 bezierPoint(const std::array<Vec3, 4>& P, float t) {
    return deCasteljauLevels(P, t)[5];
}

// Dao ham: B'(t) = 3[(1-t)^2 (P1-P0) + 2t(1-t)(P2-P1) + t^2 (P3-P2)].
// Cung bang 3 (R1 - R0) cua de Casteljau: huong doan cuoi cung chinh la tiep tuyen.
Vec3 bezierTangent(const std::array<Vec3, 4>& P, float t) {
    const float s = 1 - t;
    return ((P[1] - P[0]) * (s * s) + (P[2] - P[1]) * (2 * t * s) + (P[3] - P[2]) * (t * t)) * 3.0f;
}

std::array<Vec3, 4> bezierLine(Vec3 a, Vec3 b) {
    return {a, lerp(a, b, 1.0f / 3), lerp(a, b, 2.0f / 3), b};
}

// ============================================================================
// 2. MAT TRON XOAY (slide CG.5, Lab 05)
// ============================================================================
// Profile nam trong mat phang xy (x = ban kinh >= 0, y = do cao), quay quanh truc y.
// Quay diem (r, y, 0) mot goc theta bang Ry(theta):  (r cos theta, y, -r sin theta).
//
// PHAP TUYEN GIAI TICH (Sprint 3): tai mot diem cua profile, tiep tuyen T = (dr, dy) = B'(t).
// Phap tuyen trong mat phang profile vuong goc voi T, chon phia BEN PHAI khi di theo profile:
//   n = (dy, -dr)       (di len tren (dy > 0) o ban kinh r -> n huong ra ngoai +x)
// Roi quay n cung goc theta nhu diem. Quy uoc: profile di sao cho "ben ngoai" vat nam ben phai
// (vd thanh ngoai di tu duoi len, thanh trong di tu tren xuong).
//
// CHIEU DINH: o luoi giua vong j (duoi) va j+1 (tren), lat i va i+1:
//   a = (j, i), b = (j, i+1), c = (j+1, i+1), d = (j+1, i)
//   theta tang -> diem di theo huong -z; j tang -> di theo profile.
//   cross(b - a, d - a) cung huong voi n  =>  tam giac (a, b, c), (a, c, d) nguoc chieu kim dong ho
//   khi nhin tu phia n (dung quy uoc mesh.hpp).
//
// DINH TREN TRUC (r = 0, vd dinh qua cau): moi lat trung nhau. Neu phap tuyen tai do thang dung
// thi gom thanh MOT dinh, phap tuyen (0, +-1, 0); o luoi ke truc chi con 1 tam giac.
// (Dinh non co phap tuyen xien thi giu rieng tung lat de to bong nghieng dung.)
//
// CHO NOI 2 DOAN: neu 2 tiep tuyen lech nhau > 30 do (goc gay, vd mep nap hinh tru) thi tao
// 2 vong cung vi tri nhung khac phap tuyen, khong noi luoi giua chung -> canh sac.
namespace {

struct Ring {
    Vec3 point;    // diem tren profile (x = r, y)
    Vec3 normal;   // phap tuyen 2D cua profile (x, y), da chuan hoa
    float v;       // toa do texture doc profile
    bool connect;  // co noi luoi voi vong truoc khong
};

Vec3 profileNormal(Vec3 tangent) {
    return normalize(Vec3{tangent.y, -tangent.x, 0});
}

}  // namespace

Mesh revolve(const Profile& profile, int ringsPerSegment, int slices, const Material& material) {
    Mesh m;
    m.name = "mat tron xoay";
    m.material = material;
    if (profile.segments.empty()) return m;
    ringsPerSegment = std::max(1, ringsPerSegment);
    slices = std::max(3, slices);

    // --- Lay mau profile thanh cac vong ---
    std::vector<Ring> rings;
    const int segCount = static_cast<int>(profile.segments.size());
    const float cosSharp = std::cos(radians(30));
    for (int s = 0; s < segCount; ++s) {
        const auto& P = profile.segments[s];
        for (int j = 0; j <= ringsPerSegment; ++j) {
            const float t = static_cast<float>(j) / ringsPerSegment;
            Vec3 T = bezierTangent(P, t);
            if (length(T) < 1e-6f)  // diem dieu khien trung nhau: lay tiep tuyen gan do
                T = bezierPoint(P, std::clamp(t + 1e-3f, 0.0f, 1.0f)) - bezierPoint(P, std::clamp(t - 1e-3f, 0.0f, 1.0f));
            Ring r{bezierPoint(P, t), profileNormal(T), static_cast<float>(s * ringsPerSegment + j) / (segCount * ringsPerSegment), true};
            r.point.x = std::max(0.0f, r.point.x);
            if (rings.empty()) { r.connect = false; rings.push_back(r); continue; }
            if (j == 0) {  // cho noi voi doan truoc: cung vi tri voi vong cuoi cua doan truoc
                Ring& prev = rings.back();
                if (dot(prev.normal, r.normal) >= cosSharp) {
                    prev.normal = normalize(prev.normal + r.normal);  // noi tron: dung chung 1 vong
                    continue;
                }
                r.connect = false;  // goc gay: vong moi, khong noi luoi
            }
            rings.push_back(r);
        }
    }

    // --- Quay cac vong quanh truc y ---
    const float eps = 1e-5f;
    std::vector<int> start(rings.size());   // chi so dinh dau tien cua moi vong
    std::vector<bool> pole(rings.size());   // vong gom thanh 1 dinh tren truc
    for (std::size_t k = 0; k < rings.size(); ++k) {
        const Ring& r = rings[k];
        start[k] = static_cast<int>(m.vertices.size());
        pole[k] = r.point.x < eps && std::fabs(r.normal.y) > 0.99f;
        if (pole[k]) {
            Vertex v;
            v.position = {0, r.point.y, 0};
            v.normal = {0, r.normal.y > 0 ? 1.0f : -1.0f, 0};
            v.u = 0.5f; v.v = r.v;
            m.vertices.push_back(v);
            continue;
        }
        for (int i = 0; i <= slices; ++i) {  // lat cuoi trung lat dau (u = 1) de texture khong bi noi sai
            const float theta = 2 * kPi * i / slices;
            const float c = std::cos(theta), s = std::sin(theta);
            Vertex v;
            v.position = {r.point.x * c, r.point.y, -r.point.x * s};
            v.normal = {r.normal.x * c, r.normal.y, -r.normal.x * s};
            v.u = static_cast<float>(i) / slices; v.v = r.v;
            m.vertices.push_back(v);
        }
    }

    // --- Noi luoi giua 2 vong lien tiep ---
    for (std::size_t k = 1; k < rings.size(); ++k) {
        if (!rings[k].connect) continue;
        const bool lowPole = pole[k - 1], highPole = pole[k];
        if (lowPole && highPole) continue;
        const bool lowAxis = rings[k - 1].point.x < eps, highAxis = rings[k].point.x < eps;
        for (int i = 0; i < slices; ++i) {
            const int a = lowPole ? start[k - 1] : start[k - 1] + i;
            const int b = lowPole ? start[k - 1] : start[k - 1] + i + 1;
            const int c = highPole ? start[k] : start[k] + i + 1;
            const int d = highPole ? start[k] : start[k] + i;
            // Canh nam tren truc (2 dinh cung vi tri) thi bo tam giac suy bien.
            if (!lowAxis) m.triangles.push_back({a, b, c});
            if (!highAxis) m.triangles.push_back({a, c, d});
        }
    }
    return m;
}

// ============================================================================
// 3. MAT CO SAN (Sprint 4) — deu dung revolve()
// ============================================================================
namespace {

// Hang so de 1 doan Bezier bac 3 xap xi 1/4 duong tron (sai so ban kinh < 0,03 %).
constexpr float kArc = 0.5522847498f;  // 4/3 * tan(pi/8)

// 1/4 duong tron tam C, ban kinh R, tu goc a0 toi a1 = a0 +- 90 do (radian).
std::array<Vec3, 4> quarterArc(Vec3 C, float R, float a0, float a1) {
    const float dir = a1 > a0 ? 1.0f : -1.0f;
    const Vec3 p0 = C + Vec3{std::cos(a0), std::sin(a0), 0} * R;
    const Vec3 p3 = C + Vec3{std::cos(a1), std::sin(a1), 0} * R;
    const Vec3 t0 = Vec3{-std::sin(a0), std::cos(a0), 0} * (dir * kArc * R);  // tiep tuyen tai p0
    const Vec3 t1 = Vec3{-std::sin(a1), std::cos(a1), 0} * (dir * kArc * R);
    return {p0, p0 + t0, p3 - t1, p3};
}

}  // namespace

Mesh makeSphere(float radius, int rings, int slices, const Material& material) {
    // Nua duong tron tu cuc nam (0, -R) qua xich dao (R, 0) len cuc bac (0, R): di len -> mat ngoai ben phai.
    Profile p;
    p.segments = {quarterArc({0, 0, 0}, radius, -kPi / 2, 0), quarterArc({0, 0, 0}, radius, 0, kPi / 2)};
    Mesh m = revolve(p, std::max(1, rings / 2), slices, material);
    m.name = "qua cau";
    return m;
}

Mesh makeCylinder(float radius, float height, int slices, const Material& material) {
    // Day (tu truc ra mep) -> than (di len) -> nap (tu mep vao truc). 2 goc gay 90 do.
    Profile p;
    p.segments = {bezierLine({0, 0, 0}, {radius, 0, 0}), bezierLine({radius, 0, 0}, {radius, height, 0}),
                  bezierLine({radius, height, 0}, {0, height, 0})};
    Mesh m = revolve(p, 1, slices, material);
    m.name = "hinh tru";
    return m;
}

Mesh makeCone(float radius, float height, int slices, const Material& material) {
    Profile p;
    p.segments = {bezierLine({0, 0, 0}, {radius, 0, 0}), bezierLine({radius, 0, 0}, {0, height, 0})};
    Mesh m = revolve(p, 1, slices, material);
    m.name = "hinh non";
    return m;
}

Mesh makeTorus(float majorRadius, float minorRadius, int rings, int slices, const Material& material) {
    // Duong tron tam (R, 0) ban kinh a, di NGUOC chieu kim dong ho (duoi -> ngoai -> tren -> trong)
    // de mat ngoai cua ong nam ben phai huong di.
    const Vec3 C{majorRadius, 0, 0};
    Profile p;
    for (int q = 0; q < 4; ++q) {
        const float a0 = -kPi / 2 + q * kPi / 2;
        p.segments.push_back(quarterArc(C, minorRadius, a0, a0 + kPi / 2));
    }
    Mesh m = revolve(p, std::max(1, rings / 4), slices, material);
    m.name = "xuyen";
    return m;
}

// ============================================================================
// 4. PROFILE VAT TRUNG BAY — moi vat la mot chuoi doan Bezier noi C1
// ============================================================================
// Toa do theo met, day vat o y = 0. Vat co thanh day: thanh ngoai di len, vong qua mieng,
// thanh trong di xuong toi day trong -> nhin vao mieng binh thay long binh (khong bi thung).
Profile exhibitProfile(int index) {
    auto P = [](float x, float y) { return Vec3{x, y, 0}; };
    Profile p;
    switch (index % 5) {
        case 0:  // binh cau co thon (gom trang men)
            p.segments = {
                bezierLine(P(0, 0), P(0.09f, 0)),                                   // day
                {P(0.09f, 0), P(0.20f, 0.02f), P(0.24f, 0.16f), P(0.16f, 0.27f)},   // bung phinh
                {P(0.16f, 0.27f), P(0.10f, 0.34f), P(0.06f, 0.36f), P(0.07f, 0.45f)},  // co thon
                {P(0.07f, 0.45f), P(0.075f, 0.48f), P(0.10f, 0.49f), P(0.10f, 0.50f)},  // loe mieng
                bezierLine(P(0.10f, 0.50f), P(0.09f, 0.50f)),                       // mep mieng
                {P(0.09f, 0.50f), P(0.06f, 0.47f), P(0.055f, 0.42f), P(0.05f, 0.38f)},  // thanh trong
                bezierLine(P(0.05f, 0.38f), P(0, 0.38f)),                           // day trong
            };
            break;
        case 1:  // binh co cao (dong)
            p.segments = {
                bezierLine(P(0, 0), P(0.11f, 0)),
                {P(0.11f, 0), P(0.11f, 0.05f), P(0.19f, 0.10f), P(0.17f, 0.24f)},
                {P(0.17f, 0.24f), P(0.15f, 0.36f), P(0.05f, 0.38f), P(0.05f, 0.52f)},
                {P(0.05f, 0.52f), P(0.05f, 0.60f), P(0.09f, 0.64f), P(0.11f, 0.66f)},
                bezierLine(P(0.11f, 0.66f), P(0.10f, 0.66f)),
                {P(0.10f, 0.66f), P(0.065f, 0.635f), P(0.04f, 0.59f), P(0.035f, 0.54f)},
                bezierLine(P(0.035f, 0.54f), P(0, 0.54f)),
            };
            break;
        case 2:  // qua cau tron (nhua do) — dung makeSphere, profile nay de du phong
            p.segments = {quarterArc({0, 0.2f, 0}, 0.2f, -kPi / 2, 0), quarterArc({0, 0.2f, 0}, 0.2f, 0, kPi / 2)};
            break;
        case 3:  // bat rong mieng (gom xanh)
            p.segments = {
                bezierLine(P(0, 0), P(0.08f, 0)),
                bezierLine(P(0.08f, 0), P(0.08f, 0.03f)),
                {P(0.08f, 0.03f), P(0.20f, 0.04f), P(0.27f, 0.10f), P(0.28f, 0.18f)},
                bezierLine(P(0.28f, 0.18f), P(0.26f, 0.18f)),
                {P(0.26f, 0.18f), P(0.25f, 0.11f), P(0.19f, 0.06f), P(0.07f, 0.05f)},
                bezierLine(P(0.07f, 0.05f), P(0, 0.05f)),
            };
            break;
        default:  // lo dat nung thap, mieng hep
            p.segments = {
                bezierLine(P(0, 0), P(0.12f, 0)),
                {P(0.12f, 0), P(0.24f, 0.01f), P(0.26f, 0.22f), P(0.13f, 0.30f)},
                {P(0.13f, 0.30f), P(0.10f, 0.32f), P(0.09f, 0.33f), P(0.09f, 0.36f)},
                bezierLine(P(0.09f, 0.36f), P(0.075f, 0.36f)),
                {P(0.075f, 0.36f), P(0.075f, 0.33f), P(0.08f, 0.31f), P(0.07f, 0.28f)},
                bezierLine(P(0.07f, 0.28f), P(0, 0.28f)),
            };
            break;
    }
    return p;
}
