// PHAN B — KHACH THAM QUAN TU DI (Hao, bao tang mo rong).
#include "visitor.hpp"

#include <algorithm>
#include <cmath>

namespace {

Waypoint go(float x, float z) { return {{x, 0, z}, 0, false, {}}; }
Waypoint view(float x, float z, float tx, float tz, float pause) { return {{x, 0, z}, pause, true, {tx, 0, tz}}; }

}  // namespace

std::vector<Waypoint> museumTour() {
    return {
        // Sanh chinh: ngam 3 buc ben trai roi sang hanh lang A.
        go(2, 2.2f),
        view(-2.2f, 2.0f, -3.5f, 0.5f, 3),        // binh co thon
        view(-2.2f, -2.0f, -3.5f, -3.0f, 3),      // binh co cao
        view(-1.0f, -2.6f, 0, -4.2f, 2.5f),       // qua cau
        view(2.2f, -2.1f, 3.5f, -3.0f, 2.5f),     // bat rong mieng
        go(4.8f, -1.9f),                          // vong qua phia tren buc 4
        go(4.8f, -3.25f),
        // Hanh lang A: tranh hai ben tuong.
        view(9, -3.7f, 9, -4.5f, 3.5f),
        view(12, -2.8f, 12, -2, 3.5f),
        view(15, -3.7f, 15, -4.5f, 3.5f),
        go(19.2f, -3.25f),
        // Phong co vat: di vong cac tu kinh.
        view(21, -3.9f, 21, -5, 3),
        view(23.5f, -3.9f, 23.5f, -5, 3),
        view(23, -1.5f, 23, 0, 4),                // trong dong
        view(26, -3.9f, 26, -5, 2.5f),
        go(26.5f, -1.2f),
        view(26, 3.9f, 26, 5, 2.5f),
        view(23.5f, 3.9f, 23.5f, 5, 3),
        view(23, 1.5f, 23, 0, 3),
        view(21, 3.9f, 21, 5, 3),
        go(19.2f, 3.25f),
        // Hanh lang B (nguoc chieu x).
        view(15, 2.8f, 15, 2, 3.5f),
        view(12, 3.7f, 12, 4.5f, 3.5f),
        view(9, 2.8f, 9, 2, 3.5f),
        go(4.8f, 3.25f),
    };
}

std::vector<Waypoint> visitRoute(int plan, float lane) {
    const std::vector<Waypoint> tour = museumTour();
    std::vector<Waypoint> r = {
        go(lane * 1.2f, 14.6f),   // mep san truoc (noi xuat hien)
        go(lane * 0.6f, 11.5f),
        go(lane * 0.5f, 8.5f),    // qua cua chinh (canh cua mo ap 2 ben)
        go(lane * 0.5f, 5.4f),    // qua cong vao sanh
    };
    if (plan == 0) {              // chi xem sanh chinh
        r.insert(r.end(), tour.begin(), tour.begin() + 5);
    } else if (plan == 1) {       // sanh + hanh lang A roi quay lai
        r.insert(r.end(), tour.begin(), tour.begin() + 10);
        for (const Waypoint& w : {go(15, -3.25f), go(9, -3.25f), go(4.8f, -3.25f), go(4.8f, -1.9f), go(2, -1.6f)}) r.push_back(w);
    } else {                      // ca vong
        r.insert(r.end(), tour.begin(), tour.end());
    }
    // Ra ve: di lan ben kia de khong dam dau nguoi dang vao.
    for (const Waypoint& w : {go(-lane * 0.6f, 5.2f), go(-lane * 0.6f, 8.5f), go(-lane * 0.7f, 11.5f), go(-lane * 1.5f, 14.6f)})
        r.push_back(w);
    return r;
}

namespace {

const Outfit kOutfits[] = {
    {{0.75f, 0.22f, 0.20f}, {0.20f, 0.22f, 0.30f}, {0.10f, 0.07f, 0.05f}, {0.92f, 0.74f, 0.60f}},
    {{0.92f, 0.86f, 0.70f}, {0.35f, 0.25f, 0.18f}, {0.25f, 0.16f, 0.10f}, {0.80f, 0.60f, 0.46f}},
    {{0.22f, 0.50f, 0.32f}, {0.15f, 0.15f, 0.18f}, {0.05f, 0.05f, 0.05f}, {0.70f, 0.52f, 0.40f}},
    {{0.55f, 0.35f, 0.65f}, {0.85f, 0.85f, 0.85f}, {0.55f, 0.45f, 0.30f}, {0.95f, 0.80f, 0.68f}},
    {{0.95f, 0.65f, 0.15f}, {0.25f, 0.30f, 0.45f}, {0.12f, 0.08f, 0.06f}, {0.86f, 0.66f, 0.50f}},
    {{0.85f, 0.85f, 0.88f}, {0.12f, 0.12f, 0.14f}, {0.30f, 0.30f, 0.32f}, {0.90f, 0.72f, 0.58f}},
    {{0.20f, 0.30f, 0.55f}, {0.55f, 0.50f, 0.40f}, {0.45f, 0.25f, 0.12f}, {0.78f, 0.56f, 0.42f}},
};

Visitor makeVisitor(int seed, int startIndex) {
    Visitor v;
    const int plan = (seed * 7 + 1) % 3;
    const float lane = static_cast<float>(seed % 3 - 1) * 0.8f;
    v.route = visitRoute(plan, lane);
    startIndex = std::clamp(startIndex, 1, static_cast<int>(v.route.size()) - 1);
    v.next = startIndex;
    v.body.position = v.route[startIndex - 1].position;
    v.body.yaw = 0;  // yaw = 0 nhin theo -z: quay mat vao bao tang
    v.speed = 0.48f + 0.07f * (seed % 4);
    v.body.build(kOutfits[seed % 7]);
    return v;
}

}  // namespace

Visitor makeArrival(int seed) { return makeVisitor(seed, 1); }

std::vector<Visitor> makeVisitors(int count) {
    std::vector<Visitor> out;
    for (int i = 0; i < count; ++i) {
        // Rai deu trong phan tham quan ben trong (bo 4 diem vao dau va 4 diem ra cuoi).
        const int n = static_cast<int>(visitRoute((i * 7 + 1) % 3, 0).size());
        out.push_back(makeVisitor(i, 5 + (i * (n - 9)) / std::max(1, count)));
    }
    return out;
}

void Visitor::update(float dt, const WalkGrid& grid, const std::vector<Vec3>& others) {
    if (route.empty() || done) return;
    dt = std::clamp(dt, 0.0f, 0.1f);
    const Waypoint& wp = route[next];
    // Sang diem tiep theo; het lo trinh: di vong lai (loop) hoac da ra toi mep san (done).
    const auto advance = [&] {
        if (next + 1 < static_cast<int>(route.size())) ++next;
        else if (loop) next = 0;
        else done = true;
    };

    if (wait > 0) {  // dang dung ngam
        wait -= dt;
        if (wp.look) body.faceToward(wp.target, dt);
        else body.update(dt, {}, grid);
        if (wait <= 0) advance();
        return;
    }

    const Vec3 to{wp.position.x - body.position.x, 0, wp.position.z - body.position.z};
    const float dist = std::sqrt(to.x * to.x + to.z * to.z);
    if (dist < 0.2f) {  // toi noi
        stuck = blocked = 0;
        if (wp.pause > 0) wait = wp.pause;
        else advance();
        body.update(dt, {}, grid);
        return;
    }
    Vec3 dir = to / dist * speed;

    // Co nguoi phia truoc (trong 0,75 m, lech khong qua ~70 do) -> dung cho; cho > 1,2 giay thi buoc ngang.
    bool someoneAhead = false;
    for (const Vec3& o : others) {
        const float dx = o.x - body.position.x, dz = o.z - body.position.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        if (d > 1e-4f && d < 0.75f && (dx * to.x + dz * to.z) / (d * dist) > 0.35f) someoneAhead = true;
    }
    if (someoneAhead) {
        blocked += dt;
        dir = blocked > 1.2f ? Vec3{-dir.z, 0, dir.x} : Vec3{};  // quay 90 do: buoc sang phai
    } else {
        blocked = 0;
    }

    const Vec3 before = body.position;
    body.update(dt, dir, grid);
    const float moved = std::hypot(body.position.x - before.x, body.position.z - before.z);
    stuck = (moved < 0.2f * dt * speed) ? stuck + dt : 0;
    if (stuck > 3) {  // ket (vat can, nguoi dung chan): bo qua diem nay
        stuck = blocked = 0;
        advance();
    }
}
