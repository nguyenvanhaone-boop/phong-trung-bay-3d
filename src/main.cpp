// main.cpp — ghep 3 phan. OpenGL CHI dung de: mo cua so, nhan phim/chuot, dua Framebuffer len man hinh.
// Moi thuat toan do hoa nam trong src/modeling (A), src/scene (B), src/render (C).
//
// Phim tat:
//   1 khung day   2 Flat   3 Gouraud   4 Phong   L Phong <-> Blinn-Phong
//   B Back-face   Z Z-buffer   O Painter   V anh do sau   P chieu song song/phoi canh
//   C di bo <-> orbit   W/A/S/D di chuyen   keo chuot trai: xoay   +/- hoac con lan: phong to   Esc thoat
//   G khung day tam (GL_LINES, chi de kiem tra camera khi phan C chua xong)
//   X debug xen mat gan: to mau phan tam giac bi clipNear cat (trong khung day tam)

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "framebuffer.hpp"
#include "pipeline.hpp"
#include "platform/gl.hpp"
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

double gFps = 0;
int gFrames = 0;
auto gFpsT0 = std::chrono::steady_clock::now();
int gMouseX = 0, gMouseY = 0;
bool gDragging = false;

// CHE DO XEM TAM (phim G): ve khung day bang GL_LINES, chi de kiem tra camera va phong
// trong luc phan C (Lam) chua co bo to tam giac. Dung chung ma tran view/projection tu cai
// cua phan B, nen thay dung hinh la camera dung. Khi phan C xong: doi mac dinh thanh false.
bool gDebugWire = true;

const char* shadingName(ShadingMode m) {
    switch (m) {
        case ShadingMode::Wireframe: return "Khung day";
        case ShadingMode::Flat: return "Flat";
        case ShadingMode::Gouraud: return "Gouraud";
        case ShadingMode::Phong: return "Phong";
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
    for (const SceneObject& obj : gObjects) {
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
    renderScene(gFb, gObjects, gCamera, gLights, gSettings, gStats);

    glClear(GL_COLOR_BUFFER_BIT);
    present();
    if (gDebugWire) drawDebugWire();

    ++gFrames;
    const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - gFpsT0).count();
    if (dt >= 0.5) { gFps = gFrames / dt; gFrames = 0; gFpsT0 = std::chrono::steady_clock::now(); }

    char line1[160], line2[160];
    std::snprintf(line1, sizeof line1, "%.0f FPS | %s | Back-face %s | %s | %s", gFps, shadingName(gSettings.shading),
                  gSettings.backFace ? "bat" : "tat", gSettings.painter ? "Painter" : (gSettings.zBuffer ? "Z-buffer" : "khong khu"),
                  gCamera.projection == Projection::Perspective ? "phoi canh" : "song song");
    std::snprintf(line2, sizeof line2, "tam giac: %d vao, %d bo (back-face), %d ve | bien doi %.1f ms, to %.1f ms",
                  gStats.trianglesIn, gStats.culledBackFace, gStats.trianglesDrawn, gStats.msTransform, gStats.msRaster);
    glColor3f(1, 1, 1);
    drawText(8, gWinH - 18, line1);
    drawText(8, gWinH - 34, line2);
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
    if (gObjects.empty()) drawText(8, gWinH - 74, "Khung code OK - canh con trong (buildRoom chua cai).");

    glutSwapBuffers();
}

void reshape(int w, int h) {
    gWinW = w; gWinH = h > 0 ? h : 1;
    glViewport(0, 0, gWinW, gWinH);
}

void keyboard(unsigned char key, int, int) {
    const float step = 0.15f;
    switch (key) {
        case 27: std::exit(0);
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
                // Thu tu trong buildRoom(): 3 mat phong, roi tung cap (buc, vat).
                const Vec3 look = gCamera.eye + gCamera.forward() * 2.0f;
                float best = 1e9f;
                for (std::size_t i = 4; i < gObjects.size(); i += 2) {
                    const Vec3 p{gObjects[i].model.m[0][3], gObjects[i].model.m[1][3], gObjects[i].model.m[2][3]};
                    const float d = length(p - look);
                    if (d < best) { best = d; gCamera.target = p; }
                }
                gCamera.orbitRadius = 1.6f;
                gCamera.orbitTheta = gCamera.yaw + kPi;  // dung o phia minh dang dung
                gCamera.orbitPhi = 0.35f;
                gCamera.mode = Camera::Mode::Orbit;
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
    if (button == GLUT_LEFT_BUTTON) { gDragging = state == GLUT_DOWN; gMouseX = x; gMouseY = y; }
    // Con lan chuot: GLUT khong co ham rieng, freeglut gui con lan len/xuong thanh nut 3/4.
    // Orbit: lan len = lai gan vat. Di bo: lan len = di toi.
    if ((button == 3 || button == 4) && state == GLUT_DOWN) {
        const bool up = button == 3;
        if (gCamera.mode == Camera::Mode::Orbit) gCamera.zoom(up ? 0.9f : 1.1f);
        else gCamera.walk(up ? 0.3f : -0.3f, 0);
        glutPostRedisplay();
    }
    // TODO(Hao, Sprint 4): click khong keo = chon vat bang tia.
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
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(gWinW, gWinH);
    glutCreateWindow("Phong trung bay 3D");

    gFb.resize(960, 540);
    gObjects = buildRoom();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}
