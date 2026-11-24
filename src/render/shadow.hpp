#pragma once
// PHAN C — BONG DO PHANG VA DEN (Lam, Sprint 5: DHMTPTB3-43, DHMTPTB3-44).
#include <vector>

#include "framebuffer.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "scene/scene.hpp"

// Ma tran chieu tu diem sang L len mat phang san y = 0 (toa do dong nhat).
// Diem P di theo tia tu L qua P toi khi cham san: P' = L + t (P - L) voi P'.y = 0.
//   | Ly  -Lx  0   0  |
//   | 0    0   0   0  |       P' = S P, roi chia cho w = Ly - Py.
//   | 0   -Lz  Ly  0  |
//   | 0   -1   0   Ly |
// Chi dung khi den cao hon diem (Py < Ly): diem cao bang hoac hon den khong co bong tren san.
Mat4 planarShadowMatrix(Vec3 light);

// Tao luoi bong cua `casters` tren san cho tung den: moi dinh (the gioi) nhan S roi chia w,
// nang len `lift` m de khong tranh do sau voi san. Toa do texture tinh lai theo cach san
// dat (u, v), nen bong giu van san nhung chi con anh sang moi truong (shadowMaterial).
// Vat do bong = casters[first], casters[first + 1], ... (bo qua san, tuong, tran o dau danh sach).
void buildPlanarShadows(const std::vector<SceneObject>& casters, std::size_t first, const std::vector<Light>& lights,
                        const Material& shadowMaterial, float roomHalfX, float roomHalfZ,
                        std::vector<SceneObject>& out, float lift = 0.003f);

// Ban cho 1 den, vat do bong cho bang con tro (khong sao chep luoi). Them vao cuoi `out`.
// Dung cho bao tang nhieu khu: moi den chi do bong cac vat cung khu voi no.
void appendPlanarShadows(const std::vector<const SceneObject*>& casters, const Light& light,
                         const Material& shadowMaterial, float roomHalfX, float roomHalfZ,
                         std::vector<SceneObject>& out, float lift = 0.003f);

// Qua cau nho phat sang tai vi tri moi den (ka = mau den, kd = ks = 0 -> khong bi to bong).
std::vector<SceneObject> buildLightMarkers(const std::vector<Light>& lights, float radius = 0.1f);

// Den tha tran (bao tang mo rong): bong den phat sang + chao den 2 mat (ngoai kim loai toi, trong
// sang am) + day treo len tran cao `ceiling`. Luoi tinh tu tam bong den (goc toa do), dat bang
// translate(vi tri den). Phan tu 0 la bong den (dung de chon / keo den bang chuot).
std::vector<SceneObject> buildPendantLamp(const Light& light, float ceiling);

// Phim N (DHMTPTB3-27): ve phap tuyen dinh thanh gach ngan dai `len` m (Bresenham), chi cac dinh
// co phap tuyen huong ve mat; dung de chung minh phap tuyen giai tich khi thuyet trinh.
// Ve objects[first], objects[first + 1], ... ; dau gach mau `color`.
void drawNormals(Framebuffer& fb, const std::vector<SceneObject>& objects, std::size_t first, const Camera& camera,
                 float len, Vec3 color);
// Ban nhan danh sach con tro (bao tang: chi ve vat trung bay o cac khu dang thay).
void drawNormals(Framebuffer& fb, const std::vector<const SceneObject*>& objects, const Camera& camera,
                 float len, Vec3 color);
