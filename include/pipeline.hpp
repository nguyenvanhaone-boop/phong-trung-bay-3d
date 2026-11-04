#pragma once
// pipeline.hpp — cac kieu du lieu chay qua "duong ong" ve hinh, dung chung giua B va C.
//
//   Mesh (A) --model--> the gioi --view (B)--> he quan sat --xen mat gan (B)--> --chieu (B)--> man hinh
//        --Back-face / Painter / Z-buffer (C)--> --to dong quet + to bong (C)--> Framebuffer

#include "math3d.hpp"
#include "mesh.hpp"

enum class ShadingMode { Wireframe = 1, Flat = 2, Gouraud = 3, Phong = 4 };
enum class Projection { Perspective, Parallel };

struct RenderSettings {
    ShadingMode shading = ShadingMode::Flat;
    bool backFace = true;     // phim B
    bool zBuffer = true;      // phim Z
    bool painter = false;     // phim O (Painter thay cho Z-buffer)
    bool showDepth = false;   // phim D: hien mang do sau
    bool blinnPhong = false;
};

// Dinh trong he quan sat (sau view * model), truoc khi chieu. Dung cho buoc xen mat gan.
struct ViewVertex {
    Vec3 view;     // toa do trong he quan sat (camera o goc, nhin theo -z)
    Vec3 world;    // toa do the gioi (de tinh anh sang)
    Vec3 normal;   // phap tuyen trong the gioi, da chuan hoa
    float u = 0, v = 0;
};

// Dinh tren man hinh, dau vao cua bo to dong quet.
struct ScreenVertex {
    float x = 0, y = 0;   // pixel, (0,0) goc tren trai
    float z = 0;          // do sau = khoang cach theo truc nhin (duong, cang nho cang gan)
    Vec3 world, normal;   // de noi suy cho Phong
    Vec3 color;           // mau tai dinh (Gouraud)
    float u = 0, v = 0;
};

struct Light {
    Vec3 position{3, 4, 2};
    Vec3 color{1, 1, 1};
};

// So lieu moi khung hinh, hien len man hinh va dung cho bang FPS trong bao cao.
struct FrameStats {
    int trianglesIn = 0;
    int culledBackFace = 0;
    int clipped = 0;
    int trianglesDrawn = 0;
    double msTransform = 0, msRaster = 0;
};
