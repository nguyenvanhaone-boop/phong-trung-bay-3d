#pragma once
// framebuffer.hpp — anh do nhom tu ve (mang mau + mang do sau).
// OpenGL chi dung de dua anh nay len man hinh (present() trong src/platform/gl_present.cpp).
// Toa do pixel: (0, 0) o GOC TREN BEN TRAI, x sang phai, y xuong duoi (giong toa do man hinh trong slide).

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "math3d.hpp"

struct Framebuffer {
    int width = 0, height = 0;
    std::vector<std::uint8_t> color;  // RGBA, luu tu dong DUOI len (de glDrawPixels ve thang)
    std::vector<float> depth;         // Z-buffer: cang nho cang gan

    void resize(int w, int h) {
        width = w; height = h;
        color.assign(static_cast<std::size_t>(w) * h * 4, 0);
        depth.assign(static_cast<std::size_t>(w) * h, std::numeric_limits<float>::infinity());
    }

    void clear(Vec3 rgb) {
        const std::uint8_t r = toByte(rgb.x), g = toByte(rgb.y), b = toByte(rgb.z);
        for (std::size_t i = 0; i < color.size(); i += 4) {
            color[i] = r; color[i + 1] = g; color[i + 2] = b; color[i + 3] = 255;
        }
        std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::infinity());
    }

    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }

    // Chi so trong mang, da doi y (tren xuong) sang hang luu (duoi len).
    std::size_t index(int x, int y) const { return static_cast<std::size_t>(height - 1 - y) * width + x; }

    void setPixel(int x, int y, Vec3 rgb) {
        if (!inside(x, y)) return;
        const std::size_t i = index(x, y) * 4;
        color[i] = toByte(rgb.x); color[i + 1] = toByte(rgb.y); color[i + 2] = toByte(rgb.z); color[i + 3] = 255;
    }

    float& depthAt(int x, int y) { return depth[index(x, y)]; }

    static std::uint8_t toByte(float c) { return static_cast<std::uint8_t>(std::clamp(c, 0.0f, 1.0f) * 255.0f + 0.5f); }
};
