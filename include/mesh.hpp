#pragma once
// mesh.hpp — "hop dong" giua 3 phan:
//   A (Khang) TAO ra Mesh  ->  B (Hao) DAT Mesh vao canh  ->  C (Lam) VE Mesh.
// Chot o Sprint 1. Muon them truong thi bao ca nhom truoc, khong tu y doi ten.
//
// Quy uoc:
//   - Moi mat la mot tam giac (3 chi so dinh).
//   - Dinh cua tam giac xep NGUOC chieu kim dong ho khi nhin tu NGOAI vat
//     => phap tuyen cross(b - a, c - a) huong ra ngoai (Back-face dung quy uoc nay).
//   - normal cua dinh da chuan hoa (do 1).
//   - (u, v) trong [0, 1], dung cho texture. Mat tron xoay: u = goc quay, v = vi tri tren profile.

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "math3d.hpp"
#include "texture.hpp"

struct Vertex {
    Vec3 position;
    Vec3 normal;
    float u = 0, v = 0;
};

struct Material {
    std::string name = "gom";
    Vec3 ka{0.1f, 0.1f, 0.1f};      // he so moi truong (ambient)
    Vec3 kd{0.8f, 0.8f, 0.8f};      // he so khuech tan (diffuse) = mau nen
    Vec3 ks{0.5f, 0.5f, 0.5f};      // he so phan xa guong (specular)
    float shininess = 64.0f;        // so mu n trong cong thuc Phong
    // Texture (Sprint 5, DHMTPTB3-41): khac null thi mau nen kd duoc nhan voi texel tai (u, v).
    // shared_ptr vi nhieu vat co the dung chung mot anh, va Material duoc sao chep theo gia tri.
    std::shared_ptr<const Texture> texture;
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<std::array<int, 3>> triangles;
    Material material;
};

// Mot vat trong phong: luoi + ma tran dat vat (model matrix).
struct SceneObject {
    Mesh mesh;
    Mat4 model = Mat4::identity();
    bool visible = true;
};

// Phap tuyen mat (chua chuan hoa) theo quy uoc nguoc chieu kim dong ho.
inline Vec3 faceNormal(const Mesh& m, const std::array<int, 3>& t) {
    const Vec3 a = m.vertices[t[0]].position, b = m.vertices[t[1]].position, c = m.vertices[t[2]].position;
    return cross(b - a, c - a);
}

// Kiem tra hop le: chi so trong pham vi, khong lap dinh trong mot tam giac.
inline bool validate(const Mesh& m, std::string& error) {
    const int n = static_cast<int>(m.vertices.size());
    for (std::size_t i = 0; i < m.triangles.size(); ++i) {
        const auto& t = m.triangles[i];
        for (int k : t)
            if (k < 0 || k >= n) { error = "tam giac " + std::to_string(i) + ": chi so dinh ngoai pham vi"; return false; }
        if (t[0] == t[1] || t[1] == t[2] || t[0] == t[2]) { error = "tam giac " + std::to_string(i) + ": lap dinh"; return false; }
    }
    return true;
}
