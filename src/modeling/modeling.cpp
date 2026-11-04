// PHAN A — MO HINH HOA (Khang). Cac ham duoi day la khung: thay TODO bang thuat toan.
#include "modeling.hpp"

Vec3 bezierPoint(const std::array<Vec3, 4>& P, float t) {
    // TODO(Khang, Sprint 1): de Casteljau — noi suy tuyen tinh lap 3 lan (slide CG.5).
    (void)t;
    return P[0];
}

Vec3 bezierTangent(const std::array<Vec3, 4>& P, float t) {
    // TODO(Khang, Sprint 3): B'(t) = 3[(1-t)^2 (P1-P0) + 2t(1-t)(P2-P1) + t^2 (P3-P2)].
    (void)t;
    return P[1] - P[0];
}

std::vector<Vec3> deCasteljauLevels(const std::array<Vec3, 4>& P, float t) {
    // TODO(Khang, Sprint 1): tra ve 3 diem muc 1, 2 diem muc 2, 1 diem muc 3.
    (void)P; (void)t;
    return {};
}

Mesh revolve(const Profile& profile, int ringsPerSegment, int slices, const Material& material) {
    // TODO(Khang, Sprint 2): lay mau profile -> vong, quay Ry(2*pi*i/slices) -> lat,
    // moi o luoi chia 2 tam giac nguoc chieu kim dong ho; Sprint 3: phap tuyen giai tich.
    (void)profile; (void)ringsPerSegment; (void)slices;
    Mesh m;
    m.name = "mat tron xoay (chua cai)";
    m.material = material;
    return m;
}

Mesh makeSphere(float radius, int rings, int slices, const Material& material) {
    // TODO(Khang, Sprint 4): profile = nua duong tron ban kinh radius, goi revolve().
    (void)radius; (void)rings; (void)slices;
    Mesh m;
    m.name = "qua cau (chua cai)";
    m.material = material;
    return m;
}
