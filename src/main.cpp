// main.cpp — ghep 3 phan. OpenGL CHI dung de: mo cua so, nhan phim/chuot, dua Framebuffer len man hinh.
// Moi thuat toan do hoa nam trong src/modeling (A), src/scene (B), src/render (C).
//
// Phim tat:
//   1 khung day   2 Flat   3 Gouraud   4 Phong   L Phong <-> Blinn-Phong
//   B Back-face   Z Z-buffer   O Painter   V anh do sau   P chieu song song/phoi canh
//   C di bo <-> orbit   W/A/S/D di chuyen   keo chuot trai: xoay   +/- phong to   Esc thoat
//   G khung day tam (GL_LINES, chi de kiem tra camera khi phan C chua xong)

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

// Ve canh tam giac bang GL_LINES (xem gDebugWire). Canh cat mat phang gan thi cat bot phan sau.
void drawDebugWire() {
    const Mat4 view = gCamera.viewMatrix();
    const Mat4 proj = gCamera.projectionMatrix(static_cast<float>(gWinW) / gWinH);
    const float zn = -gCamera.nearPlane;
    glColor3f(0.45f, 0.85f, 1.0f);
    glBegin(GL_LINES);
    for (const SceneObject& obj : gObjects) {
        const Mat4 mv = view * obj.model;
        for (const auto& t : obj.mesh.triangles) {
            for (int e = 0; e < 3; ++e) {
                Vec3 a = transformPoint(mv, obj.mesh.vertices[t[e]].position);
                Vec3 b = transformPoint(mv, obj.mesh.vertices[t[(e + 1) % 3]].position);
                if (a.z > zn && b.z > zn) continue;                       // ca canh sau lung
                if (a.z > zn) a = b + (a - b) * ((zn - b.z) / (a.z - b.z));  // dua dau mut ve mat phang gan
                if (b.z > zn) b = a + (b - a) * ((zn - a.z) / (b.z - a.z));
                const Vec4 ca = proj * Vec4{a.x, a.y, a.z, 1}, cb = proj * Vec4{b.x, b.y, b.z, 1};
                glVertex2f((ca.x / ca.w * 0.5f + 0.5f) * gWinW, (ca.y / ca.w * 0.5f + 0.5f) * gWinH);
                glVertex2f((cb.x / cb.w * 0.5f + 0.5f) * gWinW, (cb.y / cb.w * 0.5f + 0.5f) * gWinH);
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
