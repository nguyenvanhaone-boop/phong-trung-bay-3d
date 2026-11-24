#pragma once
// PHAN B — NHAN VAT DIEU KHIEN (Hao, bao tang mo rong).
// Nguoi dung tu khoi co san cua phan A (mat tron xoay, cau, tru, hop). Moi bo phan la 1 SceneObject;
// dang di la cay bien doi phan cap (hierarchical transform), giong slide CG.4:
//
//   than  = T(vi tri + nhun) * Ry(-yaw)                      (ca nguoi)
//   chan  = than * T(khop hang) * Rx(+-goc vung)              (xoay quanh khop hang)
//   tay   = than * T(khop vai)  * Rz(dang tay) * Rx(-+goc vung)
//
// Goc vung = 0,55 * sin(pha) * muc di; pha tang theo quang duong da di (1 chu ky / 1,3 m),
// nen dung tuong thi chan cung dung theo.
#include <vector>

#include "mesh.hpp"
#include "museum.hpp"

constexpr float kCharacterRadius = 0.25f;  // ban kinh va cham (m)
constexpr float kWalkSpeed = 1.6f;          // m / s
constexpr float kCharacterHead = 1.55f;     // camera nhin vao diem nay (tren chan)
constexpr float kCharacterEye = 1.6f;       // goc nhin thu nhat: mat cao 1,6 m

// Mau quan ao (de nhieu nguoi trong bao tang trong khac nhau).
struct Outfit {
    Vec3 shirt{0.16f, 0.36f, 0.62f};
    Vec3 pants{0.18f, 0.18f, 0.22f};
    Vec3 hair{0.08f, 0.06f, 0.05f};
    Vec3 skin{0.88f, 0.68f, 0.54f};
};

struct Character {
    enum Part { Torso, Head, Hair, LegL, LegR, ShoeL, ShoeR, ArmL, ArmR, HandL, HandR, PartCount };

    Vec3 position{0, 0, 3.5f};  // diem giua 2 ban chan (tren san)
    float yaw = 0;              // huong mat, cung quy uoc Camera: yaw = 0 nhin theo -z
    float phase = 0;            // pha buoc chan (radian)
    float stride = 0;           // 0 = dung yen .. 1 = dang di (lam muot de khong giat)
    std::vector<SceneObject> parts;

    void build(const Outfit& look = Outfit{});
    // Dung yen va quay dan mat ve `target` (khach dung ngam tranh).
    void faceToward(Vec3 target, float dt);
    // dir: huong muon di trong the gioi (chi x, z), do dai 0..1. dt: giay tu khung truoc.
    // Nhan vat quay dan ve huong di, di cham tuong / vat can thi truot doc theo.
    void update(float dt, Vec3 dir, const WalkGrid& grid);
    // Tinh lai ma tran model cua tung bo phan tu position, yaw, phase, stride.
    void pose();
    Vec3 head() const { return position + Vec3{0, kCharacterHead, 0}; }
};
