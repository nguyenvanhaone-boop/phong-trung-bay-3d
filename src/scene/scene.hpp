#pragma once
// PHAN B — CANH VA CAMERA (Hao). Slide CG.4, Lab 04.
#include <string>
#include <vector>

#include "mesh.hpp"
#include "pipeline.hpp"

// Kich thuoc phong trung bay (don vi: met). Goc toa do o giua san.
constexpr float kRoomHalfX = 6.0f;   // phong rong 12 m theo x
constexpr float kRoomHalfZ = 6.0f;   // phong sau 12 m theo z
constexpr float kRoomHeight = 3.5f;  // tran cao 3,5 m
constexpr float kEyeHeight = 1.6f;   // mat nguoi xem cach san 1,6 m
constexpr float kWallMargin = 0.4f;  // khong cho mat di sat tuong hon 0,4 m

struct Camera {
    enum class Mode { Walk, Orbit } mode = Mode::Walk;

    // Che do di bo: vi tri mat + 2 goc nhin (radian).
    //   yaw   = goc quay quanh truc y; yaw = 0 la nhin theo huong -z.
    //   pitch = goc ngang/doc; duong la nhin len. Gioi han +-89 do.
    Vec3 eye{0, kEyeHeight, 4.5f};
    float yaw = 0, pitch = 0;

    // Che do orbit: mat nam tren mat cau tam `target`, ban kinh R, goc theta (ngang), phi (doc).
    Vec3 target{0, 1, 0};
    float orbitRadius = 3, orbitTheta = 0, orbitPhi = 0.3f;

    Projection projection = Projection::Perspective;
    float fovyDeg = 55, nearPlane = 0.1f, farPlane = 100;

    Vec3 eyePosition() const;                      // vi tri mat trong the gioi (ca 2 che do)
    Vec3 forward() const;                          // huong nhin, da chuan hoa
    Mat4 viewMatrix() const;                       // ma tran he quan sat (look-at) tu cai
    Mat4 projectionMatrix(float aspect) const;     // phoi canh hoac song song

    // Dieu khien (goi tu main.cpp).
    void walk(float forwardStep, float rightStep); // di theo huong nhin tren mat san, kep trong phong
    void rotate(float dYaw, float dPitch);         // keo chuot
    void zoom(float factor);                       // orbit: doi ban kinh
};

// Ma tran look-at: mat o E, nhin toi C, vecto huong len up.
Mat4 lookAt(Vec3 E, Vec3 C, Vec3 up);

// Hinh hop tam o goc, nua kich thuoc `half`, moi mat 4 dinh rieng (phap tuyen phang, toa do texture rieng).
// inward = true: phap tuyen va chieu dinh quay vao trong (dung cho tuong phong, nhin tu ben trong).
// faces: bit 0..5 = mat +x, -x, +y, -y, +z, -z (mac dinh ca 6 mat).
Mesh makeBox(Vec3 half, const Material& material, bool inward = false, unsigned faces = 0x3F);

// Dung phong trung bay: san, tuong, tran, 5 buc va cac vat.
// Thu tu: kRoomShellCount mat phong (san, tuong, tran), roi tung cap (buc, vat).
// objPath khac rong: nap them file .obj, dat len buc thu 6 o giua phong (nap loi thi bo qua,
// ghi ly do vao objError neu co).
constexpr int kRoomShellCount = 3;
std::vector<SceneObject> buildRoom(const std::string& objPath = "", std::string* objError = nullptr);

// ---- Chon vat bang tia (Sprint 4, DHMTPTB3-36) ----
// Ma tran nghich dao 4x4 (Gauss–Jordan). Tra ve false neu ma tran suy bien.
bool inverse(const Mat4& m, Mat4& out);

struct Ray {
    Vec3 origin;  // diem goc (the gioi)
    Vec3 dir;     // huong, da chuan hoa
};

// Tia tu mat di qua pixel (px, py) cua cua so w x h, (0,0) o goc TREN trai (nhu GLUT).
// aspect = ti le khung hinh dang hien. Dung duoc ca phoi canh lan song song.
Ray rayFromPixel(const Camera& camera, float px, float py, int w, int h, float aspect);

struct PickResult {
    int index = -1;    // chi so vat trong danh sach, -1 = khong trung vat nao
    float t = 0;       // khoang cach tu goc tia toi diem trung (m)
    Vec3 point;        // diem trung (the gioi)
};

// Giao tia voi moi tam giac MAT TRUOC cua moi vat (giong Back-face: mat quay lung bi bo qua),
// lay diem trung gan nhat.
PickResult pick(const std::vector<SceneObject>& objects, const Ray& ray);

// Hop bao (the gioi) cua vat: tam va ban kinh mat cau bao quanh.
void worldBounds(const SceneObject& object, Vec3& center, float& radius);

// Xen tam giac (he quan sat) theo mat phang gan z = -nearPlane (Sutherland–Hodgman).
// Tra ve da giac 0, 3 hoac 4 dinh; renderer chia lai thanh tam giac.
std::vector<ViewVertex> clipNear(const ViewVertex tri[3], float nearPlane);
