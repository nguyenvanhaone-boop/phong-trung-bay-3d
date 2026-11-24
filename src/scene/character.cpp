// PHAN B — NHAN VAT DIEU KHIEN (Hao, bao tang mo rong).
#include "character.hpp"

#include <algorithm>
#include <cmath>

#include "modeling/modeling.hpp"

namespace {

Material skinMat(const char* name, Vec3 kd, Vec3 ks, float n) {
    Material m;
    m.name = name;
    m.kd = kd;
    m.ka = kd * 0.2f;
    m.ks = ks;
    m.shininess = n;
    return m;
}

void append(Mesh& dst, const Mesh& src, const Mat4& M) {
    const int base = static_cast<int>(dst.vertices.size());
    for (Vertex v : src.vertices) {
        v.position = transformPoint(M, v.position);
        v.normal = normalize(transformDir(M, v.normal));
        dst.vertices.push_back(v);
    }
    for (const auto& t : src.triangles) dst.triangles.push_back({base + t[0], base + t[1], base + t[2]});
}

Mesh withName(Mesh m, const char* name) {
    m.name = name;
    return m;
}

// Khop (trong he toa do than nguoi, chan o y = 0, mat nhin -z, +x ben phai).
const Vec3 kHip{0.09f, 0.82f, 0};
const Vec3 kShoulder{0.2f, 1.39f, 0};
constexpr float kLegLength = 0.75f;
constexpr float kArmLength = 0.55f;
constexpr float kStepLength = 1.3f;  // quang duong cho 1 chu ky buoc (2 buoc)

}  // namespace

void Character::build(const Outfit& look) {
    const Material shirt = skinMat("ao", look.shirt, {0.15f, 0.15f, 0.15f}, 16);
    const Material pants = skinMat("quan", look.pants, {0.1f, 0.1f, 0.1f}, 12);
    const Material skin = skinMat("da", look.skin, {0.2f, 0.18f, 0.16f}, 24);
    const Material hair = skinMat("toc", look.hair, {0.3f, 0.3f, 0.3f}, 40);
    const Material shoe = skinMat("giay", {0.30f, 0.17f, 0.10f}, {0.35f, 0.3f, 0.25f}, 48);

    parts.assign(PartCount, SceneObject{});

    // Than: mat tron xoay (hong -> eo -> nguc -> vai), ep det theo z cho giong nguc nguoi.
    // Ep z * k thi phap tuyen phai chia z cho k roi chuan hoa (ma tran nghich dao chuyen vi).
    Profile body;
    auto P = [](float x, float y) { return Vec3{x, y, 0}; };
    body.segments = {
        bezierLine(P(0, 0.80f), P(0.15f, 0.80f)),
        {P(0.15f, 0.80f), P(0.165f, 0.90f), P(0.14f, 1.0f), P(0.15f, 1.10f)},
        {P(0.15f, 1.10f), P(0.17f, 1.22f), P(0.20f, 1.36f), P(0.16f, 1.44f)},
        {P(0.16f, 1.44f), P(0.11f, 1.48f), P(0.06f, 1.49f), P(0, 1.49f)},
    };
    Mesh torso = revolve(body, 6, 28, shirt);
    const float k = 0.62f;
    for (Vertex& v : torso.vertices) {
        v.position.z *= k;
        v.normal = normalize(Vec3{v.normal.x, v.normal.y, v.normal.z / k});
    }
    parts[Torso].mesh = withName(torso, "nhan vat - than");

    // Dau + co + mui (mui chi huong mat).
    Mesh head = makeCylinder(0.045f, 0.12f, 14, skin);
    for (Vertex& v : head.vertices) v.position.y += 1.44f;
    append(head, makeSphere(0.11f, 14, 22, skin), translate({0, 1.63f, 0}));
    append(head, makeSphere(0.022f, 6, 10, skin), translate({0, 1.615f, -0.105f}));
    parts[Head].mesh = withName(head, "nhan vat - dau");

    // Toc: qua cau lon hon dau, chi giu tam giac nam tren mat phang nghieng ve sau
    // (sau gay toc xuong thap, phia truoc chua mat).
    Mesh cap = makeSphere(0.118f, 24, 36, hair);
    const Vec3 up = normalize(Vec3{0, 1, 0.55f});
    std::vector<std::array<int, 3>> kept;
    for (const auto& t : cap.triangles) {
        bool keep = true;
        for (int i : t) keep = keep && dot(cap.vertices[i].position, up) > 0.01f;
        if (keep) kept.push_back(t);
    }
    cap.triangles = kept;
    for (Vertex& v : cap.vertices) v.position.y += 1.635f;
    parts[Hair].mesh = withName(cap, "nhan vat - toc");

    // Chan, tay: luoi dat sao cho khop (hang, vai) o goc toa do, treo xuong -y.
    Mesh leg = makeCylinder(0.066f, kLegLength, 14, pants);
    for (Vertex& v : leg.vertices) v.position.y -= kLegLength;
    parts[LegL].mesh = withName(leg, "nhan vat - chan trai");
    parts[LegR].mesh = withName(leg, "nhan vat - chan phai");

    Mesh foot = makeBox({0.055f, 0.035f, 0.10f}, shoe);
    for (Vertex& v : foot.vertices) v.position = v.position + Vec3{0, -kHip.y + 0.035f, -0.03f};
    parts[ShoeL].mesh = withName(foot, "nhan vat - giay trai");
    parts[ShoeR].mesh = withName(foot, "nhan vat - giay phai");

    Mesh arm = makeCylinder(0.045f, kArmLength, 12, shirt);
    for (Vertex& v : arm.vertices) v.position.y -= kArmLength;
    append(arm, makeSphere(0.058f, 8, 12, shirt), Mat4::identity());  // vai tron, che cho noi tay - than
    parts[ArmL].mesh = withName(arm, "nhan vat - tay trai");
    parts[ArmR].mesh = withName(arm, "nhan vat - tay phai");

    Mesh hand = makeSphere(0.05f, 8, 12, skin);
    for (Vertex& v : hand.vertices) v.position.y -= kArmLength + 0.04f;
    parts[HandL].mesh = withName(hand, "nhan vat - ban tay trai");
    parts[HandR].mesh = withName(hand, "nhan vat - ban tay phai");

    pose();
}

void Character::update(float dt, Vec3 dir, const WalkGrid& grid) {
    dt = std::clamp(dt, 0.0f, 0.1f);  // khung hinh bi treo lau: khong "dich chuyen tuc thoi"
    const float amount = std::min(1.0f, std::sqrt(dir.x * dir.x + dir.z * dir.z));
    if (amount > 0.01f) {
        // Quay dan ve huong di: huong nhin (sin yaw, -cos yaw) => yaw = atan2(x, -z).
        const float want = std::atan2(dir.x, -dir.z);
        const float diff = std::remainder(want - yaw, 2 * kPi);
        const float turn = 10.0f * dt;
        yaw = std::remainder(yaw + std::clamp(diff, -turn, turn), 2 * kPi);
    }
    const Vec3 step = Vec3{dir.x, 0, dir.z} * (kWalkSpeed * dt / std::max(amount, 1.0f));
    const Vec3 before = position;
    position = grid.slide(position, step, kCharacterRadius);
    const float moved = std::sqrt((position.x - before.x) * (position.x - before.x) + (position.z - before.z) * (position.z - before.z));
    phase = std::fmod(phase + moved / kStepLength * 2 * kPi, 2 * kPi);
    const float target = moved > 1e-4f ? 1.0f : 0.0f;
    stride += (target - stride) * std::min(1.0f, dt * 8);
    pose();
}

void Character::faceToward(Vec3 target, float dt) {
    dt = std::clamp(dt, 0.0f, 0.1f);
    const float dx = target.x - position.x, dz = target.z - position.z;
    if (dx * dx + dz * dz > 1e-6f) {
        const float diff = std::remainder(std::atan2(dx, -dz) - yaw, 2 * kPi);
        const float turn = 4.0f * dt;
        yaw = std::remainder(yaw + std::clamp(diff, -turn, turn), 2 * kPi);
    }
    stride += (0 - stride) * std::min(1.0f, dt * 8);
    pose();
}

void Character::pose() {
    if (parts.size() != PartCount) return;
    const float swing = 0.55f * stride * std::sin(phase);
    const float bob = 0.025f * stride * (1 - std::cos(2 * phase)) * 0.5f;
    const Mat4 body = translate(position + Vec3{0, bob, 0}) * rotateY(-yaw);

    parts[Torso].model = body;
    parts[Head].model = body;
    parts[Hair].model = body;
    for (int side = 0; side < 2; ++side) {
        const float s = side == 0 ? -1.0f : 1.0f;  // trai: x am
        const float legAngle = side == 0 ? swing : -swing;
        const Mat4 hip = body * translate({s * kHip.x, kHip.y, 0}) * rotateX(legAngle);
        parts[side == 0 ? LegL : LegR].model = hip;
        parts[side == 0 ? ShoeL : ShoeR].model = hip;
        const Mat4 shoulder = body * translate({s * kShoulder.x, kShoulder.y, 0}) * rotateZ(s * 0.08f) * rotateX(-0.8f * legAngle);
        parts[side == 0 ? ArmL : ArmR].model = shoulder;
        parts[side == 0 ? HandL : HandR].model = shoulder;
    }
}
