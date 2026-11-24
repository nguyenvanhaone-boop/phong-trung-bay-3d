// main.cpp — ghep 3 phan. OpenGL CHI dung de: mo cua so, nhan phim/chuot, dua Framebuffer len man hinh.
// Moi thuat toan do hoa nam trong src/modeling (A), src/scene (B), src/render (C).
//
// Phim tat:
//   W/A/S/D hoac mui ten (giu phim): di   Tab: goc nhin thu 3 (nhan vat) <-> goc nhin thu nhat
//   keo chuot trai: xoay camera   con lan, +/-: xa/gan nhan vat (hoac zoom khi xem vat)
//   Click chuot trai (khong keo): chon vat bang tia, xoay quanh vat, hien bang thong tin
//   C xem vat gan nhat <-> quay lai di bo   Esc thoat
//   1 khung day   2 Flat   3 Gouraud   4 Phong   L Phong <-> Blinn-Phong
//   B Back-face   Z Z-buffer   O Painter   V anh do sau   P chieu song song/phoi canh
//   G khung day tam (GL_LINES, chi de kiem tra camera khi phan C chua xong)
//   X debug xen mat gan: to mau phan tam giac bi clipNear cat (trong khung day tam)
//   K canh minh hoa Painter (qua cau xuyen tam phang) <-> bao tang
//   J bat/tat khach tham quan tu di (vao tu san truoc, ngam tranh / tu kinh, ra ve; tranh nhau)
//   H bong do phang xuong san  keo qua cau den bang chuot: doi cho den (trong khu cua den)
//   F muc toi uu FPS: 1 luong -> 4 luong -> 4 luong + nua do phan giai khi di chuyen
//   E trinh soan duong cong (Bezier/B-spline -> mat tron xoay)   N phap tuyen dang gach ngan
//   U bat/tat texture   I noi suy texture dung phoi canh <-> tuyen tinh tren man hinh (de so sanh)

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "framebuffer.hpp"
#include "modeling/curve_editor.hpp"
#include "modeling/editor_view.hpp"
#include "modeling/modeling.hpp"
#include "pipeline.hpp"
#include "platform/gl.hpp"
#include "platform/frame_counter.hpp"
#include "render/render.hpp"
#include "render/shadow.hpp"
#include "scene/character.hpp"
#include "scene/museum.hpp"
#include "scene/scene.hpp"
#include "scene/visitor.hpp"

namespace {

int gWinW = 960, gWinH = 540;
Framebuffer gFb;
// Toi uu FPS (DHMTPTB3-45), phim F doi muc:
//   0 = 1 luong, do phan giai day du (de so sanh "truoc toi uu")
//   1 = 4 luong
//   2 = 4 luong + nua do phan giai (480 x 270) trong luc camera/den dang di chuyen
int gOptLevel = 2;
constexpr int kFullW = 960, kFullH = 540;
std::chrono::steady_clock::time_point gLastMove;  // lan cuoi camera hoac den thay doi
std::vector<float> gLastView;                     // trang thai camera + den o khung truoc, de phat hien di chuyen

// ---- Bao tang, nhan vat, camera ----
Museum gMuseum;
Character gHero;
std::vector<Visitor> gVisitors;             // khach tham quan tu di (phim J bat/tat)
bool gShowVisitors = true;
constexpr int kVisitorCount = 7;            // so khach toi da cung luc
float gArrivalTimer = 2.0f;                 // giay toi khi khach moi den
int gArrivalSeed = 100;                     // khach moi: ke hoach / quan ao khac nhau
// ThirdPerson: camera bay sau nhan vat (lo xo khong xuyen tuong). FirstPerson: mat nhan vat.
// Inspect: xoay quanh mot vat (click hoac C); bam phim di hoac C / Tab de quay lai.
enum class ViewMode { ThirdPerson, FirstPerson, Inspect };
ViewMode gView = ViewMode::ThirdPerson;
ViewMode gPlayView = ViewMode::ThirdPerson;
float gFollowDist = 2.8f;                   // khoang cach camera mong muon sau nhan vat
bool gKeyDown[256] = {};                    // giu phim de di (khong dung lap phim cua he dieu hanh)
bool gArrowDown[4] = {};                    // len, xuong, trai, phai
std::chrono::steady_clock::time_point gLastFrame = std::chrono::steady_clock::now();

bool gShadows = true;                     // phim H: bong do phang xuong san (DHMTPTB3-44)
// Den: moi khu co den rieng (gMuseum.zones[z].lights). Danh sach phang de chon / keo bang chuot.
struct LightRef { int zone, index; };
std::vector<LightRef> gLightRefs;
std::vector<SceneObject> gMarkers;        // cac bo phan den tha tran (bong, chao, day treo)
std::vector<int> gMarkerLight;            // gMarkers[k] thuoc den gLightRefs[gMarkerLight[k]]
int gDragLight = -1;                      // den dang keo (chi so trong gLightRefs), -1 = khong keo den
// Bong: dung lai khi den doi cho hoac vat doi (gSceneVersion). Moi bong ghi den tao ra no,
// va duoc chieu sang boi CAC DEN KHAC trong khu (vung bong cua den 1 van nhan sang den 2).
struct ZoneShadows {
    std::vector<float> key;
    std::vector<SceneObject> objects;
    std::vector<int> light;
};
std::vector<ZoneShadows> gZoneShadows;
std::vector<std::vector<std::vector<Light>>> gLightsExcept;  // [khu][den] = den cua khu tru den do
std::vector<SceneObject> gHeroShadows;
std::vector<int> gHeroShadowLight;
int gSceneVersion = 0;

Camera gCamera;
RenderSettings gSettings;
FrameStats gStats;

FrameCounter gFrameCounter;
bool gRenderDemo = false;
int gMouseX = 0, gMouseY = 0;
int gDownX = 0, gDownY = 0;  // vi tri bam chuot trai: tha ra gan cho cu = click (chon vat), xa = keo (xoay)
bool gDragging = false;
int gSelected = -1;          // vat dang chon (chi so trong gMuseum.objects), -1 = chua chon
// Phim K: doi giua bao tang va canh minh hoa Painter (qua cau xuyen tam phang).
std::vector<SceneObject> gDemoObjects;
bool gPainterDemo = false;
Camera gOtherCamera;

// ---- Trinh soan duong cong (phim E, phan A: DHMTPTB3-16, -17, -20, -27, -34, -40) ----
bool gEditorMode = false;
CurveEditor gEditor;
Mesh gEditorPreview;        // revolve() cua duong cong, tinh lai khi duong cong doi
bool gEditorDirty = true;
EditorLayout gLayout;
EditorFrame gEditFrame;
int gEditDrag = -1;         // diem dang keo
int gPreset = 0;            // phim P: mau profile co san
bool gDeCasteljauRun = false;
std::chrono::steady_clock::time_point gDeCasteljauT0, gEditorT0 = std::chrono::steady_clock::now();
std::string gEditorMsg = "Dang soan mau 1 (binh co thon). Keo diem de sua, Backspace de ve tu dau, P de doi mau.";
bool gShowNormals = false;  // phim N: phap tuyen dang gach ngan (DHMTPTB3-27)
const char* const kProfileFile = "profile.txt";
const char* const kExportFile = "vat_tron_xoay.obj";

// CHE DO XEM TAM (phim G): ve khung day bang GL_LINES, chi de kiem tra camera va phong
// trong luc phan C (Lam) chua co bo to tam giac. Dung chung ma tran view/projection tu cai
// cua phan B, nen thay dung hinh la camera dung. Khi phan C xong: doi mac dinh thanh false.
bool gDebugWire = false;  // phan C da to duoc tam giac: mac dinh tat, bam G de bat lai

const std::vector<SceneObject>& shownObjects() { return gPainterDemo ? gDemoObjects : gMuseum.objects; }

// Ti le khung cua anh dang hien: khung day tam ve theo cua so, renderer ve theo framebuffer.
float shownAspect() {
    return gDebugWire ? static_cast<float>(gWinW) / gWinH : static_cast<float>(gFb.width) / gFb.height;
}

bool isShell(int i) {
    const ObjKind k = gMuseum.kind[i];
    return k == ObjKind::Floor || k == ObjKind::Wall || k == ObjKind::Ceiling;
}

int heroZone() {
    const int z = gMuseum.grid.zoneAt(gHero.position.x, gHero.position.z);
    return z >= 0 ? z : 0;
}

// Chuyen sang xoay quanh `center`, dung o phia minh dang dung (khong nhay ra sau vat).
// Mat orbit = target + R (cos(phi) sin(theta), sin(phi), cos(phi) cos(theta)) nen
// theta = atan2(dx, dz) voi (dx, dz) = huong tu target toi mat hien tai.
void orbitAround(Vec3 center, float radius) {
    const Vec3 e = gCamera.eyePosition();
    gCamera.target = center;
    gCamera.orbitRadius = radius;
    gCamera.orbitTheta = std::atan2(e.x - center.x, e.z - center.z);
    gCamera.orbitPhi = 0.35f;
    gCamera.mode = Camera::Mode::Orbit;
    gView = ViewMode::Inspect;
}

// Dat camera theo che do choi. Goc nhin thu 3: dung sau lung nhan vat (theta = -yaw).
void enterPlayView(ViewMode v) {
    gPlayView = v;
    gView = v;
    if (v == ViewMode::FirstPerson) {
        gCamera.mode = Camera::Mode::Walk;
        gCamera.yaw = gHero.yaw;
        gCamera.pitch = 0;
    } else {
        gCamera.mode = Camera::Mode::Orbit;
        gCamera.orbitTheta = -gHero.yaw;
        gCamera.orbitPhi = 0.3f;
    }
}

// Click (khong keo): ban tia qua pixel, chon vat gan nhat (xuyen qua kinh). Trung vat / buc / tranh
// thi xoay quanh no; trung san / tuong / tran thi chi hien thong tin; trung khoang trong thi bo chon.
void selectAt(int x, int y) {
    if (gPainterDemo) return;
    const Ray ray = rayFromPixel(gCamera, static_cast<float>(x), static_cast<float>(y), gWinW, gWinH, shownAspect());
    const PickResult hit = pick(gMuseum.objects, ray);
    gSelected = hit.index;
    if (hit.index >= 0 && !isShell(hit.index)) {
        const bool flat = gMuseum.kind[hit.index] == ObjKind::Painting || gMuseum.kind[hit.index] == ObjKind::Frame;
        orbitAround(gMuseum.center[hit.index], flat ? 2.0f : std::max(1.2f, 3.0f * gMuseum.radius[hit.index]));
    }
}

void drawText(int x, int y, const std::string& s);

// Bang thong tin vat dang chon, goc duoi trai.
void drawSelectionPanel() {
    if (gPainterDemo || gSelected < 0 || gSelected >= static_cast<int>(gMuseum.objects.size())) return;
    const SceneObject& o = gMuseum.objects[gSelected];
    const Material& m = o.mesh.material;
    const Vec3 c = gMuseum.center[gSelected];
    const float r = gMuseum.radius[gSelected];
    char buf[5][220];
    std::snprintf(buf[0], sizeof buf[0], "Vat dang chon: %s (%s)", o.mesh.name.c_str(), gMuseum.zones[gMuseum.zoneOf[gSelected]].name.c_str());
    std::snprintf(buf[1], sizeof buf[1], "%s", gMuseum.info[gSelected].c_str());
    std::snprintf(buf[2], sizeof buf[2], "%zu tam giac, %zu dinh | tam (%.2f, %.2f, %.2f), ban kinh bao %.2f m",
                  o.mesh.triangles.size(), o.mesh.vertices.size(), c.x, c.y, c.z, r);
    std::snprintf(buf[3], sizeof buf[3], "Vat lieu: %s | kd (%.2f, %.2f, %.2f) | ks (%.2f, %.2f, %.2f) | n = %.0f%s",
                  m.name.c_str(), m.kd.x, m.kd.y, m.kd.z, m.ks.x, m.ks.y, m.ks.z, m.shininess, m.texture ? " | co texture" : "");
    std::snprintf(buf[4], sizeof buf[4], "%s", gView == ViewMode::Inspect ? "Keo chuot: xoay quanh vat | con lan, +/-: zoom | W/A/S/D, C hoac Tab: di tiep"
                                                                         : "Click vao vat, tranh hoac buc de xem gan");
    glColor3f(1.0f, 0.9f, 0.3f);
    int line = 0;
    for (int i = 4; i >= 0; --i) {
        if (buf[i][0] == '\0') continue;
        drawText(8, 10 + 16 * line++, buf[i]);
    }
}

const char* shadingName(ShadingMode m) {
    switch (m) {
        case ShadingMode::Wireframe: return "Khung day";
        case ShadingMode::Flat: return "Flat";
        case ShadingMode::Gouraud: return "Gouraud";
        case ShadingMode::Phong: return gSettings.blinnPhong ? "Phong (Blinn)" : "Phong";
    }
    return "?";
}

void drawText(int x, int y, const std::string& s) {
    glRasterPos2i(x, y);
    for (char ch : s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ch);
}

void present() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, gWinW, 0, gWinH, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos2i(0, 0);
    glPixelZoom(static_cast<float>(gWinW) / gFb.width, static_cast<float>(gWinH) / gFb.height);
    glDrawPixels(gFb.width, gFb.height, GL_RGBA, GL_UNSIGNED_BYTE, gFb.color.data());
    glPixelZoom(1, 1);
}

void drawRenderDemo() {
    const float width = static_cast<float>(gFb.width);
    const float height = static_cast<float>(gFb.height);
    ScreenVertex triangles[2][3]{};
    triangles[0][0].x = width * 0.15f; triangles[0][0].y = height * 0.25f;
    triangles[0][1].x = width * 0.85f; triangles[0][1].y = height * 0.25f;
    triangles[0][2].x = width * 0.50f; triangles[0][2].y = height * 0.85f;
    triangles[0][0].color = {0.95f, 0.35f, 0.12f};
    triangles[1][0].x = width * 0.30f; triangles[1][0].y = height * 0.70f;
    triangles[1][1].x = width * 0.70f; triangles[1][1].y = height * 0.70f;
    triangles[1][2].x = width * 0.50f; triangles[1][2].y = height * 0.35f;
    triangles[1][0].color = {0.10f, 0.75f, 0.85f};
    RenderSettings settings;
    settings.zBuffer = false;
    const auto start = std::chrono::steady_clock::now();
    for (const auto& triangle : triangles) fillTriangle(gFb, triangle, ShadeContext{}, settings);
    gStats = FrameStats{};
    gStats.trianglesIn = gStats.trianglesDrawn = 2;
    gStats.msRaster = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

// DEBUG XEN MAT GAN (phim X): trong khung day tam, da giac bi clipNear cat thi ve mau cam,
// canh moi sinh ra nam tren mat phang gan ve mau do, duong cheo chia da giac 4 dinh thanh
// 2 tam giac ve net mo. Dung de chung minh Sutherland–Hodgman khi thuyet trinh: di sat vao buc.
bool gDebugClip = false;
int gClippedPolys = 0;  // so tam giac bi cat (khong tinh tam giac bi bo han) trong khung hinh vua ve

// Ve khung day bang GL_LINES (xem gDebugWire). Moi tam giac di qua clipNear cua phan B,
// nen day cung la cach kiem tra truc quan ham xen.
void drawDebugWire() {
    const Mat4 view = gCamera.viewMatrix();
    const Mat4 proj = gCamera.projectionMatrix(static_cast<float>(gWinW) / gWinH);
    const float zn = -gCamera.nearPlane;
    auto toScreen = [&](const ViewVertex& v) {
        const Vec4 c = proj * Vec4{v.view.x, v.view.y, v.view.z, 1};
        glVertex2f((c.x / c.w * 0.5f + 0.5f) * gWinW, (c.y / c.w * 0.5f + 0.5f) * gWinH);
    };
    auto onNear = [zn](const ViewVertex& v) { return v.view.z == zn; };  // clipNear gan dung z = -n

    gClippedPolys = 0;
    const std::vector<SceneObject>& objects = shownObjects();
    glBegin(GL_LINES);
    for (std::size_t oi = 0; oi < objects.size(); ++oi) {
        const SceneObject& obj = objects[oi];
        const bool selected = !gPainterDemo && static_cast<int>(oi) == gSelected;
        const Mat4 mv = view * obj.model;
        for (const auto& t : obj.mesh.triangles) {
            ViewVertex tri[3];
            for (int k = 0; k < 3; ++k) tri[k].view = transformPoint(mv, obj.mesh.vertices[t[k]].position);
            const std::vector<ViewVertex> poly = clipNear(tri, gCamera.nearPlane);
            if (poly.size() < 3) continue;
            bool cut = false;
            for (const ViewVertex& v : poly) cut = cut || onNear(v);
            if (cut) ++gClippedPolys;
            const bool highlight = gDebugClip && cut;

            const std::size_t n = poly.size();
            for (std::size_t e = 0; e < n; ++e) {
                const ViewVertex& a = poly[e];
                const ViewVertex& b = poly[(e + 1) % n];
                if (highlight && onNear(a) && onNear(b)) glColor3f(1.0f, 0.2f, 0.2f);  // canh tren mat phang gan
                else if (highlight) glColor3f(1.0f, 0.6f, 0.1f);
                else if (selected) glColor3f(1.0f, 0.9f, 0.3f);  // vat dang chon: vang
                else glColor3f(0.45f, 0.85f, 1.0f);
                toScreen(a);
                toScreen(b);
            }
            if (highlight && n == 4) {  // renderer chia quat: (0,1,2) va (0,2,3)
                glColor3f(0.6f, 0.35f, 0.1f);
                toScreen(poly[0]);
                toScreen(poly[2]);
            }
        }
    }
    glEnd();
}

// ---- Den va bong ----
void buildLightRefs() {
    gLightRefs.clear();
    gMarkers.clear();
    gMarkerLight.clear();
    for (int z = 0; z < static_cast<int>(gMuseum.zones.size()); ++z)
        for (int i = 0; i < static_cast<int>(gMuseum.zones[z].lights.size()); ++i) {
            if (gMuseum.zones[z].lights[i].position.y > kRoomHeight) continue;  // mat troi (san truoc): khong treo den
            gLightRefs.push_back({z, i});
            for (SceneObject& part : buildPendantLamp(gMuseum.zones[z].lights[i], kRoomHeight)) {
                gMarkers.push_back(std::move(part));
                gMarkerLight.push_back(static_cast<int>(gLightRefs.size()) - 1);
            }
        }
}

Light& lightOf(const LightRef& r) { return gMuseum.zones[r.zone].lights[r.index]; }

// Bong cua cac vat trong khu `z` (dung lai neu den va vat khong doi). Tam giac bong roi ra
// ngoai khu (qua cua sang khu ben) bi bo: khu ben co den rieng, tuong chan anh sang.
void updateZoneShadows(int z) {
    const Zone& zone = gMuseum.zones[z];
    std::vector<float> key = {static_cast<float>(gSceneVersion)};
    for (const Light& l : zone.lights) { key.push_back(l.position.x); key.push_back(l.position.y); key.push_back(l.position.z); }
    ZoneShadows& zs = gZoneShadows[z];
    if (zs.key == key) return;
    zs.key = key;
    zs.objects.clear();
    zs.light.clear();
    std::vector<const SceneObject*> casters;
    for (std::size_t i = 0; i < gMuseum.objects.size(); ++i)
        if (gMuseum.zoneOf[i] == z && gMuseum.castsShadow[i]) casters.push_back(&gMuseum.objects[i]);
    for (int li = 0; li < static_cast<int>(zone.lights.size()); ++li) {
        const std::size_t before = zs.objects.size();
        appendPlanarShadows(casters, zone.lights[li], zone.floor, kRoomHalfX, kRoomHalfZ, zs.objects);
        for (std::size_t k = before; k < zs.objects.size(); ++k) {
            Mesh& m = zs.objects[k].mesh;
            std::vector<std::array<int, 3>> kept;
            for (const auto& t : m.triangles) {
                const Vec3 c = (m.vertices[t[0]].position + m.vertices[t[1]].position + m.vertices[t[2]].position) / 3.0f;
                if (zone.area.contains(c.x, c.z)) kept.push_back(t);
            }
            m.triangles = std::move(kept);
            zs.light.push_back(li);
        }
    }
}

void updateLightsExcept() {
    gLightsExcept.resize(gMuseum.zones.size());
    for (std::size_t z = 0; z < gMuseum.zones.size(); ++z) {
        const std::vector<Light>& all = gMuseum.zones[z].lights;
        gLightsExcept[z].assign(all.size(), {});
        for (std::size_t i = 0; i < all.size(); ++i)
            for (std::size_t j = 0; j < all.size(); ++j)
                if (j != i) gLightsExcept[z][i].push_back(all[j]);
    }
}

std::size_t totalLights() { return gLightRefs.size(); }

// Ve bao tang: moi vat voi den cua khu no; nhan vat (goc nhin thu 3); bong; qua cau den.
// Tat ca trong MOT lan goi renderItems: vat duc truoc, kinh sau cung (dung thu tu tron mau).
void renderWorld() {
    if (gPainterDemo) {
        renderScene(gFb, gDemoObjects, gCamera, gMuseum.zones[0].lights, gSettings, gStats);
        return;
    }
    const Museum& mu = gMuseum;
    std::vector<DrawItem> items;
    items.reserve(mu.objects.size() + 64);
    for (std::size_t i = 0; i < mu.objects.size(); ++i)
        items.push_back({&mu.objects[i], &mu.zones[mu.zoneOf[i]].lights, mu.center[i], mu.radius[i]});

    // Nguoi trong bao tang: nhan vat (tru goc nhin thu nhat) va khach. Moi nguoi lay den cua khu dang dung.
    std::vector<std::pair<const Character*, int>> people;
    auto zoneAt = [&](Vec3 p) { const int z = mu.grid.zoneAt(p.x, p.z); return z >= 0 ? z : 0; };
    if (gView != ViewMode::FirstPerson) people.push_back({&gHero, heroZone()});
    if (gShowVisitors)
        for (const Visitor& v : gVisitors) people.push_back({&v.body, zoneAt(v.body.position)});
    for (const auto& [person, z] : people)
        for (const SceneObject& part : person->parts)
            items.push_back({&part, &mu.zones[z].lights, person->position + Vec3{0, 0.9f, 0}, 1.0f});

    updateLightsExcept();
    if (gShadows && gSettings.zBuffer && !gSettings.painter) {
        for (int z = 0; z < static_cast<int>(mu.zones.size()); ++z) {
            updateZoneShadows(z);
            const ZoneShadows& zs = gZoneShadows[z];
            const Rect& a = mu.zones[z].area;
            const Vec3 c = a.center();
            const float r = 0.5f * std::sqrt((a.x1 - a.x0) * (a.x1 - a.x0) + (a.z1 - a.z0) * (a.z1 - a.z0));
            for (std::size_t k = 0; k < zs.objects.size(); ++k)
                items.push_back({&zs.objects[k], &gLightsExcept[z][zs.light[k]], c, r});
        }
        // Bong nguoi: dung lai moi khung (nguoi di chuyen). gHeroShadows giu bong cua moi nguoi,
        // gHeroShadowLight ghi (khu, den) tao ra tung bong.
        gHeroShadows.clear();
        gHeroShadowLight.clear();
        std::vector<Vec3> owner;
        for (const auto& [person, z] : people) {
            std::vector<const SceneObject*> parts;
            for (const SceneObject& p : person->parts) parts.push_back(&p);
            const Zone& zone = mu.zones[z];
            for (int li = 0; li < static_cast<int>(zone.lights.size()); ++li) {
                const std::size_t before = gHeroShadows.size();
                appendPlanarShadows(parts, zone.lights[li], zone.floor, kRoomHalfX, kRoomHalfZ, gHeroShadows, 0.004f);
                for (std::size_t k = before; k < gHeroShadows.size(); ++k) {
                    gHeroShadowLight.push_back(z * 64 + li);
                    owner.push_back(person->position);
                }
            }
        }
        for (std::size_t k = 0; k < gHeroShadows.size(); ++k)
            items.push_back({&gHeroShadows[k], &gLightsExcept[gHeroShadowLight[k] / 64][gHeroShadowLight[k] % 64], owner[k], 3.0f});
    }

    for (std::size_t k = 0; k < gMarkers.size(); ++k) {
        const LightRef& ref = gLightRefs[gMarkerLight[k]];
        const Vec3 p = lightOf(ref).position;
        gMarkers[k].model = translate(p);
        items.push_back({&gMarkers[k], &mu.zones[ref.zone].lights, p + Vec3{0, 0.4f, 0}, 0.8f});
    }

    renderItems(gFb, items, gCamera, gSettings, gStats);

    if (gShowNormals) {
        std::vector<const SceneObject*> exhibits;
        for (std::size_t i = 0; i < mu.objects.size(); ++i)
            if (mu.kind[i] == ObjKind::Exhibit || mu.kind[i] == ObjKind::Stand) exhibits.push_back(&mu.objects[i]);
        drawNormals(gFb, exhibits, gCamera, 0.04f, {0.3f, 1.0f, 0.5f});
    }
}

// ---- Di chuyen nhan vat va camera (moi khung hinh) ----
void updatePlay() {
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - gLastFrame).count();
    gLastFrame = now;
    if (gPainterDemo || gRenderDemo) return;

    const float fwd = float(gKeyDown['w'] || gArrowDown[0]) - float(gKeyDown['s'] || gArrowDown[1]);
    const float side = float(gKeyDown['d']) - float(gKeyDown['a']);
    const float turn = float(gArrowDown[3]) - float(gArrowDown[2]);  // mui ten trai/phai: quay
    const bool moving = fwd != 0 || side != 0;
    if (moving && gView == ViewMode::Inspect) {
        enterPlayView(gPlayView);
        gSelected = -1;
    }
    if (turn != 0 && gView != ViewMode::Inspect) gCamera.rotate(turn * 1.8f * std::min(dt, 0.1f), 0);

    Vec3 ahead, right;
    if (gView == ViewMode::FirstPerson) {
        ahead = {std::sin(gCamera.yaw), 0, -std::cos(gCamera.yaw)};
        right = {std::cos(gCamera.yaw), 0, std::sin(gCamera.yaw)};
    } else {
        // Huong camera nhin tren mat san: nguoc voi (sin(theta), cos(theta)).
        ahead = {-std::sin(gCamera.orbitTheta), 0, -std::cos(gCamera.orbitTheta)};
        right = {std::cos(gCamera.orbitTheta), 0, -std::sin(gCamera.orbitTheta)};
    }
    Vec3 dir = ahead * fwd + right * side;
    const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    if (len > 1) dir = dir / len;
    if (gView == ViewMode::Inspect) dir = {};
    const Vec3 heroBefore = gHero.position;
    gHero.update(dt, dir, gMuseum.grid);
    // Khong di xuyen qua khach: buoc moi lam 2 nguoi chong len nhau (< 0,5 m) va gan hon truoc -> huy buoc.
    if (gShowVisitors)
        for (const Visitor& v : gVisitors) {
            const Vec3 d = gHero.position - v.body.position, d0 = heroBefore - v.body.position;
            if (d.x * d.x + d.z * d.z < 0.25f && d.x * d.x + d.z * d.z < d0.x * d0.x + d0.z * d0.z) {
                gHero.position = heroBefore;
                gHero.pose();
                break;
            }
        }
    if (gShowVisitors) {
        // Nguoi ra, nguoi vao: khach di het lo trinh (ra toi mep san) thi bien mat; cu 4-9 giay
        // co mot khach moi den o mep san neu chua du kVisitorCount nguoi.
        gVisitors.erase(std::remove_if(gVisitors.begin(), gVisitors.end(), [](const Visitor& v) { return v.done; }), gVisitors.end());
        gArrivalTimer -= dt;
        if (gArrivalTimer <= 0 && static_cast<int>(gVisitors.size()) < kVisitorCount) {
            gVisitors.push_back(makeArrival(gArrivalSeed++));
            gArrivalTimer = 4.0f + static_cast<float>(gArrivalSeed * 37 % 50) / 10.0f;
        }
        for (std::size_t i = 0; i < gVisitors.size(); ++i) {
            std::vector<Vec3> others = {gHero.position};
            for (std::size_t j = 0; j < gVisitors.size(); ++j)
                if (j != i) others.push_back(gVisitors[j].body.position);
            gVisitors[i].update(dt, gMuseum.grid, others);
        }
    }
    if (gView == ViewMode::FirstPerson) {  // nhin thu nhat: nhan vat quay theo chuot, khong theo huong di
        gHero.yaw = gCamera.yaw;
        gHero.pose();
    }

    if (gView == ViewMode::FirstPerson) {
        gCamera.mode = Camera::Mode::Walk;
        gCamera.eye = gHero.position + Vec3{0, kCharacterEye, 0};
    } else if (gView == ViewMode::ThirdPerson) {
        gCamera.mode = Camera::Mode::Orbit;
        gCamera.orbitPhi = std::clamp(gCamera.orbitPhi, -0.25f, 1.2f);
        gCamera.target = gHero.head();
        const Vec3 d{std::cos(gCamera.orbitPhi) * std::sin(gCamera.orbitTheta), std::sin(gCamera.orbitPhi),
                     std::cos(gCamera.orbitPhi) * std::cos(gCamera.orbitTheta)};
        gCamera.orbitRadius = std::max(0.3f, springArm(gMuseum.grid, gCamera.target, d, gFollowDist, kRoomHeight));
    }
}

// ---- Trinh soan duong cong ----
Mesh revolveForEditor(const Material& m) {
    Mesh mesh = revolve(gEditor.toProfile(), 10, 48, m);
    mesh.name = "vat tu soan";
    return mesh;
}

void displayEditor() {
    if (gFb.width != kFullW || gFb.height != kFullH) gFb.resize(kFullW, kFullH);
    if (gEditorDirty) {
        Material m;
        m.name = "gom soan";
        m.kd = {0.85f, 0.75f, 0.55f}; m.ka = m.kd * 0.15f; m.ks = {0.5f, 0.5f, 0.5f}; m.shininess = 48;
        gEditorPreview = revolveForEditor(m);
        gEditorDirty = false;
    }
    const auto now = std::chrono::steady_clock::now();
    gEditFrame.previewAngle = 0.4f * std::chrono::duration<float>(now - gEditorT0).count();
    const int segs = gEditor.segmentCount();
    gEditFrame.deCasteljau = (gDeCasteljauRun && segs > 0)
        ? std::fmod(0.5f * std::chrono::duration<float>(now - gDeCasteljauT0).count(), float(segs))  // 2 giay / doan
        : -1.0f;
    gEditFrame.selected = gEditDrag;
    gFb.clear({0, 0, 0});
    drawEditor(gFb, gEditor, gEditorPreview, gLayout, gEditFrame);

    glClear(GL_COLOR_BUFFER_BIT);
    present();
    char l1[200], l2[200];
    std::snprintf(l1, sizeof l1, "TRINH SOAN DUONG CONG (E: ve bao tang) | %s (S) | %zu diem, %d doan | %zu tam giac | %.0f FPS",
                  gEditor.kind == CurveEditor::Kind::Bezier ? "Bezier noi C1" : "B-spline bac 3 deu",
                  gEditor.points.size(), segs, gEditorPreview.triangles.size(), gFrameCounter.fps());
    std::snprintf(l2, sizeof l2, "Trai: them/keo diem | Phai: xoa | T: de Casteljau | N: phap tuyen | P: mau | W/R: luu/tai %s | X: xuat .obj | Enter: dat len buc",
                  kProfileFile);
    glColor3f(1, 1, 1);
    drawText(8, gWinH - 18, l1);
    drawText(8, gWinH - 34, l2);
    glColor3f(1.0f, 0.85f, 0.3f);
    drawText(8, gWinH - 50, gEditorMsg);
    if (gDeCasteljauRun && segs > 0) {
        char l3[120];
        const float g = gEditFrame.deCasteljau;
        std::snprintf(l3, sizeof l3, "de Casteljau: doan %d, t = %.2f | xanh la: muc 1 (3 diem), xanh duong: muc 2, do: diem tren cong", int(g) + 1, g - std::floor(g));
        drawText(8, gWinH - 66, l3);
    }
    glutSwapBuffers();
    gFrameCounter.record();
}

// Enter: thay vat dang chon (hoac vat dau tien o sanh) bang mat tron xoay vua soan, dat day vat len mat buc.
void placeOnStand() {
    if (gPainterDemo) { gEditorMsg = "Dang o canh Painter: ve bao tang (K) truoc roi dat vat."; return; }
    if (gEditor.segmentCount() == 0) { gEditorMsg = "Chua co doan cong nao."; return; }
    const Museum& mu = gMuseum;
    int target = -1;
    if (gSelected >= 0 && mu.kind[gSelected] == ObjKind::Exhibit && mu.standOf[gSelected] >= 0) target = gSelected;
    for (std::size_t i = 0; target < 0 && i < mu.objects.size(); ++i)
        if (mu.kind[i] == ObjKind::Exhibit && mu.standOf[i] >= 0) target = static_cast<int>(i);
    if (target < 0) return;
    SceneObject& obj = gMuseum.objects[target];
    const SceneObject& stand = gMuseum.objects[mu.standOf[target]];
    float top = -1e9f, minY = 1e9f;
    for (const Vertex& v : stand.mesh.vertices) top = std::max(top, transformPoint(stand.model, v.position).y);
    Mesh mesh = revolveForEditor(obj.mesh.material);
    for (const Vertex& v : mesh.vertices) minY = std::min(minY, v.position.y);
    obj.mesh = std::move(mesh);
    obj.model = translate({obj.model.m[0][3], top - minY, obj.model.m[2][3]});
    gMuseum.updateBounds(target);
    ++gSceneVersion;  // bong phai dung lai
    gSelected = target;
    gEditorMsg = "Da dat vat len buc (" + obj.mesh.material.name + ", " + mu.zones[mu.zoneOf[target]].name + "). Bam E de xem.";
}

void editorKey(unsigned char key) {
    std::string err;
    switch (key) {
        case 27: std::exit(0);
        case 'e': case 'E': gEditorMode = false; break;
        case 's': case 'S':
            gEditor.kind = gEditor.kind == CurveEditor::Kind::Bezier ? CurveEditor::Kind::BSpline : CurveEditor::Kind::Bezier;
            gEditorMsg = gEditor.kind == CurveEditor::Kind::Bezier ? "Bezier: vuong = diem noi (tren cong), tron = diem dieu khien."
                                                                   : "B-spline: keo 1 diem chi doi cac doan gan no; tu dong lien tuc C2.";
            gEditorDirty = true;
            break;
        case 't': case 'T':
            gDeCasteljauRun = !gDeCasteljauRun;
            gDeCasteljauT0 = std::chrono::steady_clock::now();
            break;
        case 'n': case 'N': gEditFrame.showNormals = !gEditFrame.showNormals; break;
        case 'p': case 'P':
            gEditor.fromProfile(exhibitProfile(gPreset));
            gEditorMsg = "Mau " + std::to_string(gPreset + 1) + "/5.";
            gPreset = (gPreset + 1) % 5;
            gEditorDirty = true;
            break;
        case 'w': case 'W':
            gEditorMsg = gEditor.save(kProfileFile, err) ? std::string("Da luu ") + kProfileFile : err;
            break;
        case 'r': case 'R':
            if (gEditor.load(kProfileFile, err)) { gEditorMsg = std::string("Da tai ") + kProfileFile; gEditorDirty = true; }
            else gEditorMsg = err;
            break;
        case 'x': case 'X':
            gEditorMsg = exportObj(revolveForEditor(Material{}), kExportFile, err)
                ? std::string("Da xuat ") + kExportFile + " (mo duoc bang Blender, hoac ./build/phong_trung_bay " + kExportFile + ")"
                : err;
            break;
        case 8: case 127:  // Backspace / Delete: xoa het
            gEditor.points.clear();
            gEditorMsg = "Da xoa het diem.";
            gEditorDirty = true;
            break;
        case 13: case 10: placeOnStand(); break;
        default: return;
    }
}

void editorMouse(int button, int state, int x, int y) {
    const float fx = x * float(kFullW) / gWinW, fy = y * float(kFullH) / gWinH;  // cua so -> framebuffer
    const Vec3 p = gLayout.toProfile(fx, fy);
    const float pick = 10.0f / gLayout.scale;                                     // ban kinh chon 10 pixel
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) { gEditDrag = -1; return; }
    if (state != GLUT_DOWN || fx >= gLayout.panelWidth) return;
    if (button == GLUT_LEFT_BUTTON) {
        int i = gEditor.nearest(p, pick);
        if (i < 0) i = gEditor.addPoint(p);
        gEditDrag = i;
        gEditorDirty = true;
    } else if (button == GLUT_RIGHT_BUTTON) {
        const int i = gEditor.nearest(p, pick);
        if (i >= 0) { gEditor.removePoint(i); gEditorDirty = true; }
    }
}

void editorMotion(int x, int y) {
    if (gEditDrag < 0) return;
    const float fx = x * float(kFullW) / gWinW, fy = y * float(kFullH) / gWinH;
    gEditor.movePoint(gEditDrag, gLayout.toProfile(std::min(fx, float(gLayout.panelWidth)), fy));
    gEditorDirty = true;
}

// Dang di chuyen? So vi tri mat, huong nhin, kieu chieu va vi tri den voi khung truoc.
bool viewMoving() {
    const Vec3 e = gCamera.eyePosition(), f = gCamera.forward();
    std::vector<float> now = {e.x, e.y, e.z, f.x, f.y, f.z, float(gCamera.projection == Projection::Perspective), gHero.phase};
    for (const LightRef& r : gLightRefs) { now.push_back(lightOf(r).position.x); now.push_back(lightOf(r).position.z); }
    const auto t = std::chrono::steady_clock::now();
    if (now != gLastView) { gLastView = now; gLastMove = t; }
    return std::chrono::duration<double>(t - gLastMove).count() < 0.15;  // dung tay 150 ms -> ve lai net
}

const char* viewName() {
    switch (gView) {
        case ViewMode::ThirdPerson: return "goc nhin thu 3";
        case ViewMode::FirstPerson: return "goc nhin thu nhat";
        case ViewMode::Inspect: return "xem vat (orbit)";
    }
    return "?";
}

void display() {
    if (gEditorMode) { displayEditor(); return; }
    updatePlay();
    // Chon do phan giai va so luong theo muc toi uu.
    gSettings.threads = gOptLevel == 0 ? 1 : 4;
    const bool half = gOptLevel == 2 && !gRenderDemo && viewMoving();
    const int w = half ? kFullW / 2 : kFullW, h = half ? kFullH / 2 : kFullH;
    if (gFb.width != w || gFb.height != h) gFb.resize(w, h);  // present() tu phong anh len vua cua so
    // Nen = bau troi: chi lo ra o san truoc ngoai troi (khong tran) va phia tren tuong rao.
    gFb.clear(gRenderDemo || gPainterDemo ? Vec3{0.12f, 0.12f, 0.14f} : Vec3{0.56f, 0.73f, 0.92f});
    if (gRenderDemo) drawRenderDemo();
    else renderWorld();

    glClear(GL_COLOR_BUFFER_BIT);
    present();
    if (!gRenderDemo && gDebugWire) drawDebugWire();

    char line1[200], line2[220];
    std::snprintf(line1, sizeof line1, "%.0f FPS | %s | Back-face %s | %s | %s | texture %s", gFrameCounter.fps(), shadingName(gSettings.shading),
                  gSettings.backFace ? "bat" : "tat", gSettings.painter ? "Painter" : (gSettings.zBuffer ? "Z-buffer" : "khong khu"),
                  gCamera.projection == Projection::Perspective ? "phoi canh" : "song song",
                  !gSettings.texture ? "tat" : (gSettings.perspectiveCorrect ? "dung phoi canh" : "tuyen tinh (SAI)"));
    if (gRenderDemo)
        std::snprintf(line1, sizeof line1, "%.0f FPS | Demo scanline | Flat color | No depth test | T: scene", gFrameCounter.fps());
    static const char* const optNames[3] = {"1 luong", "4 luong", "4 luong + nua do phan giai khi di"};
    std::snprintf(line2, sizeof line2, "tam giac: %d vao, %d bo, %d ve | %d vat ngoai khung nhin | bien doi %.1f ms, to %.1f ms | %dx%d, F: %s | %zu den, bong %s (H)",
                  gStats.trianglesIn, gStats.culledBackFace, gStats.trianglesDrawn, gStats.objectsCulled, gStats.msTransform, gStats.msRaster,
                  gFb.width, gFb.height, optNames[gOptLevel], gPainterDemo ? gMuseum.zones[0].lights.size() : totalLights(), gShadows ? "bat" : "tat");
    glColor3f(1, 1, 1);
    drawText(8, gWinH - 18, line1);
    drawText(8, gWinH - 34, line2);
    if (!gRenderDemo) {
        const Vec3 e = gCamera.eyePosition();
        char line3[220];
        if (gPainterDemo)
            std::snprintf(line3, sizeof line3, "canh Painter | mat (%.1f, %.1f, %.1f)%s", e.x, e.y, e.z, gDebugWire ? " | G: tat khung day tam" : "");
        else
            std::snprintf(line3, sizeof line3, "%s (Tab) | khu: %s | nhan vat (%.1f, %.1f) | %zu khach tu di (J: %s) | mat (%.1f, %.1f, %.1f)%s",
                          viewName(), gMuseum.zones[heroZone()].name.c_str(), gHero.position.x, gHero.position.z,
                          gVisitors.size(), gShowVisitors ? "bat" : "tat", e.x, e.y, e.z,
                          gDebugWire ? " | G: tat khung day tam" : "");
        drawText(8, gWinH - 50, line3);
        if (gDebugWire && gDebugClip) {
            char line4[160];
            std::snprintf(line4, sizeof line4, "X: debug xen mat gan (near = %.2f m) | %d tam giac bi cat | cam = bi cat, do = canh tren mat phang gan",
                          gCamera.nearPlane, gClippedPolys);
            glColor3f(1.0f, 0.6f, 0.1f);
            drawText(8, gWinH - 66, line4);
        }
        drawSelectionPanel();
        if (gPainterDemo) {
            glColor3f(1.0f, 0.9f, 0.3f);
            drawText(8, gWinH - 82, gSettings.painter ? "Canh Painter: O dang BAT -> tam phang va qua cau ve sai thu tu (cat nhau). O: ve lai bang Z-buffer | K: ve bao tang"
                                                      : "Canh Painter: dang dung Z-buffer (dung). Bam O de thay Painter sai | K: ve bao tang");
        }
        if (gSettings.showDepth && (gSettings.painter || !gSettings.zBuffer)) {
            glColor3f(1.0f, 0.6f, 0.1f);
            drawText(8, gWinH - 98, "V: Painter / tat Z-buffer thi khong ghi do sau -> anh do sau trong. Bat Z (va tat O) de xem.");
        }
    }

    glutSwapBuffers();
    gFrameCounter.record();
}

void refresh() {
    glutPostRedisplay();
}

void reshape(int w, int h) {
    gWinW = w > 0 ? w : 1; gWinH = h > 0 ? h : 1;
    glViewport(0, 0, gWinW, gWinH);
    glutPostRedisplay();
}

// Xem vat trung bay / tranh gan diem dang nhin nhat (phim C).
void inspectNearest() {
    const Vec3 look = gCamera.eyePosition() + gCamera.forward() * 2.0f;
    float best = 1e9f;
    int bestIdx = -1;
    for (std::size_t i = 0; i < gMuseum.objects.size(); ++i) {
        if (gMuseum.kind[i] != ObjKind::Exhibit && gMuseum.kind[i] != ObjKind::Painting) continue;
        const float d = length(gMuseum.center[i] - look);
        if (d < best) { best = d; bestIdx = static_cast<int>(i); }
    }
    if (bestIdx < 0) return;
    gSelected = bestIdx;
    const bool flat = gMuseum.kind[bestIdx] == ObjKind::Painting;
    orbitAround(gMuseum.center[bestIdx], flat ? 2.0f : 1.6f);
}

void keyboardUp(unsigned char key, int, int) {
    gKeyDown[std::tolower(key)] = false;
}

void keyboard(unsigned char key, int, int) {
    if (gEditorMode) { editorKey(key); glutPostRedisplay(); return; }
    const unsigned char lower = static_cast<unsigned char>(std::tolower(key));
    switch (lower) {
        case 27: std::exit(0);
        case 'w': case 'a': case 's': case 'd':  // giu phim: updatePlay() di moi khung hinh
            gKeyDown[lower] = true;
            break;
        case 'e': gEditorMode = true; std::fill(std::begin(gKeyDown), std::end(gKeyDown), false); break;
        case 'n': gShowNormals = !gShowNormals; break;
        case 't': gRenderDemo = !gRenderDemo; break;
        case '1': gSettings.shading = ShadingMode::Wireframe; break;
        case '2': gSettings.shading = ShadingMode::Flat; break;
        case '3': gSettings.shading = ShadingMode::Gouraud; break;
        case '4': gSettings.shading = ShadingMode::Phong; break;
        case 'l': gSettings.blinnPhong = !gSettings.blinnPhong; break;
        case 'b': gSettings.backFace = !gSettings.backFace; break;
        case 'z': gSettings.zBuffer = !gSettings.zBuffer; break;
        case 'o': gSettings.painter = !gSettings.painter; break;
        case 'v': gSettings.showDepth = !gSettings.showDepth; break;
        case 'p':
            gCamera.projection = gCamera.projection == Projection::Perspective ? Projection::Parallel : Projection::Perspective;
            break;
        case 'c':
            if (gPainterDemo) break;
            if (gView == ViewMode::Inspect) enterPlayView(gPlayView);
            else inspectNearest();
            break;
        case '\t':
            if (gPainterDemo) break;
            enterPlayView(gView == ViewMode::Inspect ? gPlayView
                          : (gPlayView == ViewMode::ThirdPerson ? ViewMode::FirstPerson : ViewMode::ThirdPerson));
            break;
        case '+': case '=':
            if (gView == ViewMode::ThirdPerson) gFollowDist = std::max(1.2f, gFollowDist * 0.9f);
            else gCamera.zoom(0.9f);
            break;
        case '-':
            if (gView == ViewMode::ThirdPerson) gFollowDist = std::min(6.0f, gFollowDist * 1.1f);
            else gCamera.zoom(1.1f);
            break;
        case 'g': gDebugWire = !gDebugWire; break;
        case 'x': gDebugClip = !gDebugClip; break;
        case 'k':
            if (gDemoObjects.empty()) {  // lan dau: dung canh Painter, camera orbit nhin xien
                gDemoObjects = buildPainterDemo();
                gOtherCamera.mode = Camera::Mode::Orbit;
                gOtherCamera.target = kPainterDemoCenter;
                gOtherCamera.orbitRadius = 2.6f;
                gOtherCamera.orbitTheta = 0.5f;
                gOtherCamera.orbitPhi = 0.4f;
            }
            std::swap(gCamera, gOtherCamera);
            gPainterDemo = !gPainterDemo;
            gSelected = -1;
            break;
        case 'h': gShadows = !gShadows; break;
        case 'j': gShowVisitors = !gShowVisitors; break;
        case 'f': gOptLevel = (gOptLevel + 1) % 3; break;
        case 'u': gSettings.texture = !gSettings.texture; break;
        case 'i': gSettings.perspectiveCorrect = !gSettings.perspectiveCorrect; break;
        default: return;
    }
    glutPostRedisplay();
}

int arrowIndex(int key) {
    switch (key) {
        case GLUT_KEY_UP: return 0;
        case GLUT_KEY_DOWN: return 1;
        case GLUT_KEY_LEFT: return 2;
        case GLUT_KEY_RIGHT: return 3;
    }
    return -1;
}

// Mui ten: len/xuong = di toi/lui, trai/phai = quay (giu phim).
void special(int key, int, int) {
    const int i = arrowIndex(key);
    if (i >= 0) gArrowDown[i] = true;
}

void specialUp(int key, int, int) {
    const int i = arrowIndex(key);
    if (i >= 0) gArrowDown[i] = false;
}

void mouse(int button, int state, int x, int y) {
    if (gEditorMode) { editorMouse(button, state, x, y); glutPostRedisplay(); return; }
    if (button == GLUT_LEFT_BUTTON) {
        gDragging = state == GLUT_DOWN;
        gMouseX = x; gMouseY = y;
        if (state == GLUT_DOWN) {
            gDownX = x; gDownY = y;
            // Bam trung qua cau den -> keo den thay vi xoay camera.
            if (!gPainterDemo) {
                const Ray ray = rayFromPixel(gCamera, static_cast<float>(x), static_cast<float>(y), gWinW, gWinH, shownAspect());
                const PickResult light = pick(gMarkers, ray);
                const PickResult scene = pick(gMuseum.objects, ray);
                gDragLight = (light.index >= 0 && (scene.index < 0 || light.t < scene.t)) ? gMarkerLight[light.index] : -1;
            }
        }
        if (state == GLUT_UP && gDragLight >= 0) { gDragLight = -1; return; }
        // Tha chuot gan cho bam (<= 3 pixel) = click: chon vat bang tia.
        if (state == GLUT_UP && std::abs(x - gDownX) <= 3 && std::abs(y - gDownY) <= 3) {
            selectAt(x, y);
            glutPostRedisplay();
        }
    }
    // Con lan chuot: GLUT khong co ham rieng, freeglut gui con lan len/xuong thanh nut 3/4.
    if ((button == 3 || button == 4) && state == GLUT_DOWN) {
        const bool up = button == 3;
        if (gView == ViewMode::ThirdPerson && !gPainterDemo) gFollowDist = std::clamp(gFollowDist * (up ? 0.9f : 1.1f), 1.2f, 6.0f);
        else if (gCamera.mode == Camera::Mode::Orbit) gCamera.zoom(up ? 0.9f : 1.1f);
        glutPostRedisplay();
    }
}

void motion(int x, int y) {
    if (gEditorMode) { editorMotion(x, y); glutPostRedisplay(); return; }
    if (!gDragging) return;
    if (gDragLight >= 0) {
        // Keo den tren mat phang ngang y = do cao den: giao tia chuot voi mat phang do, kep trong khu cua den.
        const LightRef ref = gLightRefs[gDragLight];
        Light& light = lightOf(ref);
        const Rect& a = gMuseum.zones[ref.zone].area;
        const Ray ray = rayFromPixel(gCamera, static_cast<float>(x), static_cast<float>(y), gWinW, gWinH, shownAspect());
        if (std::fabs(ray.dir.y) > 1e-4f) {
            const float t = (light.position.y - ray.origin.y) / ray.dir.y;
            if (t > 0) {
                const Vec3 p = ray.origin + ray.dir * t;
                light.position.x = std::clamp(p.x, a.x0 + 0.3f, a.x1 - 0.3f);
                light.position.z = std::clamp(p.z, a.z0 + 0.3f, a.z1 - 0.3f);
            }
        }
        glutPostRedisplay();
        return;
    }
    const float k = 0.005f;  // radian moi pixel keo
    gCamera.rotate((x - gMouseX) * k, -(y - gMouseY) * k);
    gMouseX = x; gMouseY = y;
    glutPostRedisplay();
}

}  // namespace

int main(int argc, char** argv) {
    for (int argument = 1; argument < argc; ) {
        if (std::string(argv[argument]) == "--render-demo") {
            gRenderDemo = true;
            for (int next = argument; next < argc; ++next) argv[next] = argv[next + 1];
            --argc;
        } else {
            ++argument;
        }
    }
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(gWinW, gWinH);
    glutCreateWindow("Bao tang 3D");

    gFb.resize(kFullW, kFullH);
    // Vat o buc giua sanh: file .obj truyen vao dong lenh, mac dinh am tra Utah.
    //   ./build/phong_trung_bay duong/dan/vat.obj
    const std::string objPath = argc > 1 ? argv[1] : "assets/teapot.obj";
    std::string objError;
    gMuseum = buildMuseum(objPath, &objError);
    if (!objError.empty()) std::fprintf(stderr, "Khong nap duoc .obj: %s\n", objError.c_str());
    for (const std::string& w : gMuseum.warnings) std::fprintf(stderr, "Canh bao: %s\n", w.c_str());
    gZoneShadows.resize(gMuseum.zones.size());
    buildLightRefs();
    gHero.position = gMuseum.spawn;
    gHero.build();
    gVisitors = makeVisitors(kVisitorCount);
    enterPlayView(ViewMode::ThirdPerson);
    gEditor.fromProfile(exhibitProfile(0));  // trinh soan mo san mau binh co thon
    gPreset = 1;

    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    gFrameCounter = FrameCounter{};
    glutIdleFunc(refresh);
    glutMainLoop();
    return 0;
}
