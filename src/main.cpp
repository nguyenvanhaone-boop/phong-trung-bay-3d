// main.cpp — ghep 3 phan. OpenGL CHI dung de: mo cua so, nhan phim/chuot, dua Framebuffer len man hinh.
// Moi thuat toan do hoa nam trong src/modeling (A), src/scene (B), src/render (C).
//
// Phim tat:
//   1 khung day   2 Flat   3 Gouraud   4 Phong   L Phong <-> Blinn-Phong
//   B Back-face   Z Z-buffer   O Painter   V anh do sau   P chieu song song/phoi canh
//   C di bo <-> orbit   W/A/S/D di chuyen   keo chuot trai: xoay   +/- hoac con lan: phong to   Esc thoat
//   G khung day tam (GL_LINES, chi de kiem tra camera khi phan C chua xong)
//   Click chuot trai (khong keo): chon vat bang tia, orbit quanh vat, hien bang thong tin
//   X debug xen mat gan: to mau phan tam giac bi clipNear cat (trong khung day tam)
//   U bat/tat texture   I noi suy texture dung phoi canh <-> tuyen tinh tren man hinh (de so sanh)

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "framebuffer.hpp"
#include "pipeline.hpp"
#include "platform/gl.hpp"
#include "platform/frame_counter.hpp"
#include "render/render.hpp"
#include "scene/scene.hpp"

namespace {

int gWinW = 960, gWinH = 540;
Framebuffer gFb;
std::vector<SceneObject> gObjects;
std::vector<Light> gLights = {Light{}};
Camera gCamera;
RenderSettings gSettings;
FrameStats gStats;

FrameCounter gFrameCounter;
bool gRenderDemo = false;
int gMouseX = 0, gMouseY = 0;
int gDownX = 0, gDownY = 0;  // vi tri bam chuot trai: tha ra gan cho cu = click (chon vat), xa = keo (xoay)
bool gDragging = false;
int gSelected = -1;          // vat dang chon (chi so trong gObjects), -1 = chua chon

// CHE DO XEM TAM (phim G): ve khung day bang GL_LINES, chi de kiem tra camera va phong
// trong luc phan C (Lam) chua co bo to tam giac. Dung chung ma tran view/projection tu cai
// cua phan B, nen thay dung hinh la camera dung. Khi phan C xong: doi mac dinh thanh false.
bool gDebugWire = true;

// Ti le khung cua anh dang hien: khung day tam ve theo cua so, renderer ve theo framebuffer.
float shownAspect() {
    return gDebugWire ? static_cast<float>(gWinW) / gWinH : static_cast<float>(gFb.width) / gFb.height;
}

// Chuyen sang orbit quanh `center`, dung o phia minh dang dung (khong nhay ra sau vat).
// Mat orbit = target + R (cos(phi) sin(theta), sin(phi), cos(phi) cos(theta)) nen
// theta = atan2(dx, dz) voi (dx, dz) = huong tu target toi mat hien tai.
void orbitAround(Vec3 center, float radius) {
    const Vec3 e = gCamera.eyePosition();
    gCamera.target = center;
    gCamera.orbitRadius = radius;
    gCamera.orbitTheta = std::atan2(e.x - center.x, e.z - center.z);
    gCamera.orbitPhi = 0.35f;
    gCamera.mode = Camera::Mode::Orbit;
}

// Click (khong keo): ban tia qua pixel, chon vat gan nhat. Trung vat trung bay hoac buc thi
// orbit quanh no; trung san/tuong/tran thi chi hien thong tin; trung khoang trong thi bo chon.
void selectAt(int x, int y) {
    const Ray ray = rayFromPixel(gCamera, static_cast<float>(x), static_cast<float>(y), gWinW, gWinH, shownAspect());
    const PickResult hit = pick(gObjects, ray);
    gSelected = hit.index;
    if (hit.index >= kRoomShellCount) {
        Vec3 c;
        float r;
        worldBounds(gObjects[hit.index], c, r);
        orbitAround(c, std::max(1.2f, 3.0f * r));
    }
}

void drawText(int x, int y, const std::string& s);

// Bang thong tin vat dang chon, goc duoi trai.
void drawSelectionPanel() {
    if (gSelected < 0 || gSelected >= static_cast<int>(gObjects.size())) return;
    const SceneObject& o = gObjects[gSelected];
    const Material& m = o.mesh.material;
    Vec3 c;
    float r;
    worldBounds(o, c, r);
    char buf[4][160];
    std::snprintf(buf[0], sizeof buf[0], "Vat dang chon: %s", o.mesh.name.c_str());
    std::snprintf(buf[1], sizeof buf[1], "%zu tam giac, %zu dinh | tam (%.2f, %.2f, %.2f), ban kinh bao %.2f m",
                  o.mesh.triangles.size(), o.mesh.vertices.size(), c.x, c.y, c.z, r);
    std::snprintf(buf[2], sizeof buf[2], "Vat lieu: %s | kd (%.2f, %.2f, %.2f) | ks (%.2f, %.2f, %.2f) | n = %.0f",
                  m.name.c_str(), m.kd.x, m.kd.y, m.kd.z, m.ks.x, m.ks.y, m.ks.z, m.shininess);
    std::snprintf(buf[3], sizeof buf[3], "%s", gSelected >= kRoomShellCount ? "Keo chuot: xoay quanh vat | con lan, +/-: zoom | C: ve di bo"
                                                                             : "Click vao buc hoac vat de xoay quanh no");
    glColor3f(1.0f, 0.9f, 0.3f);
    for (int i = 0; i < 4; ++i) drawText(8, 10 + 16 * (3 - i), buf[i]);
}

const char* shadingName(ShadingMode m) {
    switch (m) {
        case ShadingMode::Wireframe: return "Khung day";
        case ShadingMode::Flat: return "Flat";
        case ShadingMode::Gouraud: return "Gouraud (flat fallback)";
        case ShadingMode::Phong: return "Phong (flat fallback)";
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
    glBegin(GL_LINES);
    for (std::size_t oi = 0; oi < gObjects.size(); ++oi) {
        const SceneObject& obj = gObjects[oi];
        const bool selected = static_cast<int>(oi) == gSelected;
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

void display() {
    gFb.clear({0.12f, 0.12f, 0.14f});
    if (gRenderDemo) drawRenderDemo();
    else renderScene(gFb, gObjects, gCamera, gLights, gSettings, gStats);

    glClear(GL_COLOR_BUFFER_BIT);
    present();
    if (!gRenderDemo && gDebugWire) drawDebugWire();

    char line1[160], line2[160];
    std::snprintf(line1, sizeof line1, "%.0f FPS | %s | Back-face %s | %s | %s | texture %s", gFrameCounter.fps(), shadingName(gSettings.shading),
                  gSettings.backFace ? "bat" : "tat", gSettings.painter ? "Painter" : (gSettings.zBuffer ? "Z-buffer" : "khong khu"),
                  gCamera.projection == Projection::Perspective ? "phoi canh" : "song song",
                  !gSettings.texture ? "tat" : (gSettings.perspectiveCorrect ? "dung phoi canh" : "tuyen tinh (SAI)"));
    if (gRenderDemo)
        std::snprintf(line1, sizeof line1, "%.0f FPS | Demo scanline | Flat color | No depth test | T: scene", gFrameCounter.fps());
    std::snprintf(line2, sizeof line2, "tam giac: %d vao, %d bo (back-face), %d ve | bien doi %.1f ms, to %.1f ms",
                  gStats.trianglesIn, gStats.culledBackFace, gStats.trianglesDrawn, gStats.msTransform, gStats.msRaster);
    glColor3f(1, 1, 1);
    drawText(8, gWinH - 18, line1);
    drawText(8, gWinH - 34, line2);
    if (!gRenderDemo) {
    const Vec3 e = gCamera.eyePosition();
    char line3[160];
    std::snprintf(line3, sizeof line3, "camera %s | mat (%.1f, %.1f, %.1f) | yaw %.0f, pitch %.0f do%s",
                  gCamera.mode == Camera::Mode::Walk ? "di bo" : "orbit", e.x, e.y, e.z,
                  gCamera.yaw * 180 / kPi, gCamera.pitch * 180 / kPi, gDebugWire ? " | G: tat khung day tam" : "");
    drawText(8, gWinH - 50, line3);
    if (gDebugWire && gDebugClip) {
        char line4[160];
        std::snprintf(line4, sizeof line4, "X: debug xen mat gan (near = %.2f m) | %d tam giac bi cat | cam = bi cat, do = canh tren mat phang gan",
                      gCamera.nearPlane, gClippedPolys);
        glColor3f(1.0f, 0.6f, 0.1f);
        drawText(8, gWinH - 66, line4);
    }
    if (gObjects.empty()) drawText(8, gWinH - 74, "Canh con trong (buildRoom chua cai). Nhan T: demo scanline.");
    drawSelectionPanel();
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

void keyboard(unsigned char key, int, int) {
    const float step = 0.15f;
    switch (key) {
        case 27: std::exit(0);
        case 't': case 'T': gRenderDemo = !gRenderDemo; break;
        case '1': gSettings.shading = ShadingMode::Wireframe; break;
        case '2': gSettings.shading = ShadingMode::Flat; break;
        case '3': gSettings.shading = ShadingMode::Gouraud; break;
        case '4': gSettings.shading = ShadingMode::Phong; break;
        case 'l': case 'L': gSettings.blinnPhong = !gSettings.blinnPhong; break;
        case 'b': case 'B': gSettings.backFace = !gSettings.backFace; break;
        case 'z': case 'Z': gSettings.zBuffer = !gSettings.zBuffer; break;
        case 'o': case 'O': gSettings.painter = !gSettings.painter; break;
        case 'v': case 'V': gSettings.showDepth = !gSettings.showDepth; break;
        case 'p': case 'P':
            gCamera.projection = gCamera.projection == Projection::Perspective ? Projection::Parallel : Projection::Perspective;
            break;
        case 'c': case 'C':
            if (gCamera.mode == Camera::Mode::Walk) {
                // Sang orbit quanh vat trung bay gan nhat voi diem dang nhin (cach mat 2 m).
                // Thu tu trong buildRoom(): kRoomShellCount mat phong, roi tung cap (buc, vat).
                const Vec3 look = gCamera.eye + gCamera.forward() * 2.0f;
                float best = 1e9f;
                int bestIdx = -1;
                for (std::size_t i = kRoomShellCount + 1; i < gObjects.size(); i += 2) {
                    const Vec3 p{gObjects[i].model.m[0][3], gObjects[i].model.m[1][3], gObjects[i].model.m[2][3]};
                    const float d = length(p - look);
                    if (d < best) { best = d; bestIdx = static_cast<int>(i); }
                }
                if (bestIdx >= 0) {
                    const SceneObject& o = gObjects[bestIdx];
                    gSelected = bestIdx;
                    orbitAround({o.model.m[0][3], o.model.m[1][3], o.model.m[2][3]}, 1.6f);
                }
            } else {
                gCamera.mode = Camera::Mode::Walk;
            }
            break;
        // Di theo huong nhin (Camera::walk tu kep trong phong).
        case 'w': case 'W': gCamera.walk(step, 0); break;
        case 's': case 'S': gCamera.walk(-step, 0); break;
        case 'a': case 'A': gCamera.walk(0, -step); break;
        case 'd': case 'D': gCamera.walk(0, step); break;
        case '+': case '=': gCamera.zoom(0.9f); break;
        case '-': gCamera.zoom(1.1f); break;
        case 'g': case 'G': gDebugWire = !gDebugWire; break;
        case 'x': case 'X': gDebugClip = !gDebugClip; break;
        case 'u': case 'U': gSettings.texture = !gSettings.texture; break;
        case 'i': case 'I': gSettings.perspectiveCorrect = !gSettings.perspectiveCorrect; break;
        default: return;
    }
    glutPostRedisplay();
}

void special(int key, int, int) {
    // Phim mui ten: len/xuong = di toi/lui, trai/phai = quay dau.
    if (key == GLUT_KEY_UP) gCamera.walk(0.15f, 0);
    if (key == GLUT_KEY_DOWN) gCamera.walk(-0.15f, 0);
    if (key == GLUT_KEY_LEFT) gCamera.rotate(-0.05f, 0);
    if (key == GLUT_KEY_RIGHT) gCamera.rotate(0.05f, 0);
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        gDragging = state == GLUT_DOWN;
        gMouseX = x; gMouseY = y;
        if (state == GLUT_DOWN) { gDownX = x; gDownY = y; }
        // Tha chuot gan cho bam (<= 3 pixel) = click: chon vat bang tia.
        if (state == GLUT_UP && std::abs(x - gDownX) <= 3 && std::abs(y - gDownY) <= 3) {
            selectAt(x, y);
            glutPostRedisplay();
        }
    }
    // Con lan chuot: GLUT khong co ham rieng, freeglut gui con lan len/xuong thanh nut 3/4.
    // Orbit: lan len = lai gan vat. Di bo: lan len = di toi.
    if ((button == 3 || button == 4) && state == GLUT_DOWN) {
        const bool up = button == 3;
        if (gCamera.mode == Camera::Mode::Orbit) gCamera.zoom(up ? 0.9f : 1.1f);
        else gCamera.walk(up ? 0.3f : -0.3f, 0);
        glutPostRedisplay();
    }
}

void motion(int x, int y) {
    if (!gDragging) return;
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
    glutCreateWindow("Phong trung bay 3D");

    gFb.resize(960, 540);
    // Vat o buc giua: file .obj truyen vao dong lenh, mac dinh am tra Utah.
    //   ./build/phong_trung_bay duong/dan/vat.obj
    const std::string objPath = argc > 1 ? argv[1] : "assets/teapot.obj";
    std::string objError;
    gObjects = buildRoom(objPath, &objError);
    if (!objError.empty()) std::fprintf(stderr, "Khong nap duoc .obj: %s\n", objError.c_str());

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    gFrameCounter = FrameCounter{};
    glutIdleFunc(refresh);
    glutMainLoop();
    return 0;
}
