// main.cpp — ghep 3 phan. OpenGL CHI dung de: mo cua so, nhan phim/chuot, dua Framebuffer len man hinh.
// Moi thuat toan do hoa nam trong src/modeling (A), src/scene (B), src/render (C).
//
// Phim tat:
//   1 khung day   2 Flat   3 Gouraud   4 Phong   L Phong <-> Blinn-Phong
//   B Back-face   Z Z-buffer   O Painter   V anh do sau   P chieu song song/phoi canh
//   C di bo <-> orbit   W/A/S/D di chuyen   keo chuot trai: xoay   +/- phong to   Esc thoat

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

void display() {
    gFb.clear({0.12f, 0.12f, 0.14f});
    renderScene(gFb, gObjects, gCamera, gLights, gSettings, gStats);

    glClear(GL_COLOR_BUFFER_BIT);
    present();

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
    if (gObjects.empty()) drawText(8, gWinH - 58, "Khung code OK - canh con trong (buildRoom chua cai).");

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
            gCamera.mode = gCamera.mode == Camera::Mode::Walk ? Camera::Mode::Orbit : Camera::Mode::Walk;
            break;
        // TODO(Hao, Sprint 2): di chuyen theo huong nhin (yaw) thay vi theo truc toa do.
        case 'w': case 'W': gCamera.eye.z -= step; break;
        case 's': case 'S': gCamera.eye.z += step; break;
        case 'a': case 'A': gCamera.eye.x -= step; break;
        case 'd': case 'D': gCamera.eye.x += step; break;
        case '+': case '=': gCamera.orbitRadius *= 0.9f; break;
        case '-': gCamera.orbitRadius *= 1.1f; break;
        default: return;
    }
    glutPostRedisplay();
}

void special(int key, int, int) {
    // Phim mui ten: lam giong W/A/S/D.
    if (key == GLUT_KEY_RIGHT) gCamera.eye.x += 0.15f;
    if (key == GLUT_KEY_LEFT) gCamera.eye.x -= 0.15f;
    if (key == GLUT_KEY_UP) gCamera.eye.z -= 0.15f;
    if (key == GLUT_KEY_DOWN) gCamera.eye.z += 0.15f;
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) { gDragging = state == GLUT_DOWN; gMouseX = x; gMouseY = y; }
    // TODO(Hao, Sprint 4): click khong keo = chon vat bang tia.
}

void motion(int x, int y) {
    if (!gDragging) return;
    const float k = 0.005f;
    gCamera.yaw += (x - gMouseX) * k;
    gCamera.pitch -= (y - gMouseY) * k;
    gCamera.orbitTheta += (x - gMouseX) * k;
    gCamera.orbitPhi += (y - gMouseY) * k;
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
