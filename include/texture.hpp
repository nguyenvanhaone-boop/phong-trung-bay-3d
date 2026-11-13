#pragma once
// texture.hpp — anh texture dung chung giua B (nap anh, gan cho vat lieu) va C (lay mau khi to).
// Them o Sprint 5 (DHMTPTB3-41).
//
// Quy uoc toa do texture (giong .obj va OpenGL):
//   u: 0 = canh trai anh, 1 = canh phai
//   v: 0 = canh DUOI anh, 1 = canh tren   (hang 0 cua anh nam o tren, nen phai lat v)
//   Ngoai [0, 1] thi lap lai (repeat): u = 1.25 lay giong u = 0.25. Hinh hop dung u, v theo met
//   nen anh lap lai moi 1 m tren san va buc.
#include <algorithm>
#include <cmath>
#include <vector>

#include "math3d.hpp"

struct Texture {
    int width = 0, height = 0;
    std::vector<Vec3> texels;  // RGB trong [0, 1], hang 0 = hang tren cung cua anh

    const Vec3& at(int x, int y) const { return texels[static_cast<std::size_t>(y) * width + x]; }

    // Lay mau song tuyen (bilinear): tron 4 texel gan nhat theo khoang cach.
    // Tam texel (i, j) nam o u = (i + 0.5) / width.
    Vec3 sample(float u, float v) const {
        if (width <= 0 || height <= 0) return {1, 1, 1};
        const float fx = (u - std::floor(u)) * width - 0.5f;          // repeat roi doi sang don vi texel
        const float fy = (1 - (v - std::floor(v))) * height - 0.5f;   // lat v: v = 0 la hang duoi
        const float x0f = std::floor(fx), y0f = std::floor(fy);
        const float tx = fx - x0f, ty = fy - y0f;
        auto wrap = [](int i, int n) { return ((i % n) + n) % n; };
        const int x0 = wrap(static_cast<int>(x0f), width), x1 = wrap(x0 + 1, width);
        const int y0 = wrap(static_cast<int>(y0f), height), y1 = wrap(y0 + 1, height);
        const Vec3 top = at(x0, y0) * (1 - tx) + at(x1, y0) * tx;
        const Vec3 bottom = at(x0, y1) * (1 - tx) + at(x1, y1) * tx;
        return top * (1 - ty) + bottom * ty;
    }
};
