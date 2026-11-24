#pragma once
// PHAN B — BAO TANG MO RONG (Hao). Tu 1 phong thanh 4 khu:
//
//      z
//      ^   -6 +------------+          hanh lang tranh A (dan gian)        +-----------+
//      |      |            |==========================================|           |
//      |      |   SANH     |  z = -4.5 .. -2                           |  PHONG    |
//      |      |   CHINH    |                                           |  CO VAT   |
//      |      | (phong cu) |  z = 2 .. 4.5                             | (tu kinh) |
//      |      |            |==========================================|           |
//      |    6 +------------+          hanh lang tranh B (chau Au)       +-----------+
//      |                +--+  sanh don (x -2..2, z 6..10): cong cot + dam, cua chinh o z = 10 (mo san)
//      |             +--------+  san truoc ngoai troi (x -5..5, z 10..15): mat tien da, nang, bau troi
//      +----------> x     -6           6                              18          28
//
// Cho 2 khu giap nhau tu sinh khung cua (2 cot + dam ngang). Tuong kem trang tri: giay dan tuong
// hoa van, op go chan tuong, nep ngang, phao tran.
// Mat bang = cac hinh chu nhat (Zone). Tuong KHONG ve tay: chia san thanh luoi o 0,25 m, o nao
// nam trong mot khu la "di duoc"; giua 1 o di duoc va 1 o khong di duoc la mot doan tuong
// (mat huong ve phia di duoc). Cho 2 khu giap nhau tu nhien thanh cua thong. Cung luoi do dung
// de va cham (nhan vat, camera khong di xuyen tuong).
#include <string>
#include <vector>

#include "mesh.hpp"
#include "pipeline.hpp"
#include "scene.hpp"

struct Rect {
    float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    bool contains(float x, float z) const { return x >= x0 && x <= x1 && z >= z0 && z <= z1; }
    Vec3 center() const { return {(x0 + x1) * 0.5f, 0, (z0 + z1) * 0.5f}; }
};

struct Zone {
    std::string name;
    Rect area;
    std::vector<Light> lights;  // den cua khu: chi chieu sang vat trong khu (tuong chan sang)
    Material floor;             // vat lieu san (bong do dung lai, chi doi den)
    bool outdoor = false;       // ngoai troi: khong tran, tuong da tron (khong op go, khong phao)
};

// Luoi o vuong tren mat san (nhin tu tren xuong).
struct WalkGrid {
    float x0 = 0, z0 = 0, cell = 0.25f;
    int nx = 0, nz = 0;
    std::vector<int> zone;                // khu cua o, -1 = ngoai bao tang (trong tuong)
    std::vector<unsigned char> blocked;   // 1 = co vat can dung tren o (buc, tu kinh)

    int zoneAt(float x, float z) const;
    // Hinh vuong tam (x, z), nua canh `margin` nam tron trong cac khu (khong cham tuong)?
    bool insideWalls(float x, float z, float margin) const;
    // Nhu tren va khong cham vat can: dung cho nhan vat (ban kinh `radius`).
    bool free(float x, float z, float radius) const;
    void block(const Rect& r);
    // Di tu `from` them `delta` (chi x, z): bi chan thi truot doc tuong (thu rieng x, rieng z).
    Vec3 slide(Vec3 from, Vec3 delta, float radius) const;
};

// Camera theo sau (goc nhin thu 3): di tu `target` theo huong `dir` (da chuan hoa) toi da `wanted` m,
// dung lai truoc khi cham tuong, san hoac tran -> camera khong bao gio nhin xuyen tuong.
float springArm(const WalkGrid& grid, Vec3 target, Vec3 dir, float wanted, float ceiling, float margin = 0.2f);

enum class ObjKind : unsigned char { Floor, Wall, Ceiling, Stand, Exhibit, Painting, Frame, Glass, Decor };

struct Museum {
    std::vector<SceneObject> objects;
    std::vector<int> zoneOf;             // khu cua tung vat
    std::vector<ObjKind> kind;
    std::vector<bool> castsShadow;       // co do bong xuong san khong
    std::vector<int> standOf;            // vat trung bay -> chi so buc / be duoi no, -1 neu khong co
    std::vector<std::string> info;       // mo ta hien o bang thong tin (ten tranh, tac gia, nam...)
    std::vector<Vec3> center;            // mat cau bao (the gioi) de bo vat ngoai hinh chop nhin
    std::vector<float> radius;
    std::vector<Zone> zones;
    WalkGrid grid;
    Vec3 spawn{0, 0, 12.8f};             // cho nhan vat dung luc dau (san truoc, nhin vao cua chinh)
    std::vector<std::string> warnings;   // anh tranh khong nap duoc... (canh van chay)

    int add(SceneObject object, int zone, ObjKind k, bool shadow, std::string description = "", int stand = -1);
    void updateBounds(int i);
    int paintingCount() const;
};

// Kich thuoc mat bang (m).
constexpr Rect kHallArea{-6, -6, 6, 6};
constexpr Rect kCorridorA{6, -4.5f, 18, -2};
constexpr Rect kCorridorB{6, 2, 18, 4.5f};
constexpr Rect kArtifactRoom{18, -6, 28, 6};
constexpr Rect kLobby{-2, 6, 2, 10};  // sanh don sau cong chinh (phia z lon cua sanh), cua ra vao o z = 10
constexpr Rect kPorch{-5, 10, 5, 15};  // san truoc ngoai troi; khach den / ve o mep z = 15

// Dung ca bao tang. Sanh chinh lay nguyen phong cu (buildRoom: 5 buc + buc giua nap .obj),
// chi thay vo phong bang san / tuong / tran sinh tu mat bang.
Museum buildMuseum(const std::string& objPath = "", std::string* objError = nullptr);
