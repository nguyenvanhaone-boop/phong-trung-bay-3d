#pragma once
// math3d.hpp — vecto va ma tran dung chung cho ca nhom (chot o Sprint 1, khong doi ten truong).
// Quy uoc: he toa do tay phai, truc y huong len, ma tran 4x4 luu theo hang: m[hang][cot].
// Diem nhan ma tran theo kieu cot: p' = M * p (giong slide CG.4).

#include <array>
#include <cmath>

constexpr float kPi = 3.14159265358979323846f;

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, Vec3 a) { return a * s; }
inline Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3 mul(Vec3 a, Vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }  // nhan tung thanh phan (mau)
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 normalize(Vec3 a) {
    const float l = length(a);
    return l > 0 ? a / l : a;
}

struct Vec4 {
    float x = 0, y = 0, z = 0, w = 1;
};

struct Mat4 {
    std::array<std::array<float, 4>, 4> m{};

    static Mat4 identity() {
        Mat4 r;
        for (int i = 0; i < 4; ++i) r.m[i][i] = 1;
        return r;
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

inline Vec4 operator*(const Mat4& a, Vec4 p) {
    const float v[4] = {p.x, p.y, p.z, p.w};
    float o[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i)
        for (int k = 0; k < 4; ++k) o[i] += a.m[i][k] * v[k];
    return {o[0], o[1], o[2], o[3]};
}

// Bien doi diem (w = 1) va vecto huong (w = 0).
inline Vec3 transformPoint(const Mat4& a, Vec3 p) {
    const Vec4 r = a * Vec4{p.x, p.y, p.z, 1};
    return {r.x, r.y, r.z};
}
inline Vec3 transformDir(const Mat4& a, Vec3 d) {
    const Vec4 r = a * Vec4{d.x, d.y, d.z, 0};
    return {r.x, r.y, r.z};
}

// Cac phep bien doi co ban (da lam o Lab 04, chep lai de dung chung).
inline Mat4 translate(Vec3 t) {
    Mat4 r = Mat4::identity();
    r.m[0][3] = t.x; r.m[1][3] = t.y; r.m[2][3] = t.z;
    return r;
}
inline Mat4 scale(Vec3 s) {
    Mat4 r = Mat4::identity();
    r.m[0][0] = s.x; r.m[1][1] = s.y; r.m[2][2] = s.z;
    return r;
}
inline Mat4 rotateX(float rad) {
    Mat4 r = Mat4::identity();
    const float c = std::cos(rad), s = std::sin(rad);
    r.m[1][1] = c; r.m[1][2] = -s; r.m[2][1] = s; r.m[2][2] = c;
    return r;
}
inline Mat4 rotateY(float rad) {
    Mat4 r = Mat4::identity();
    const float c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c; r.m[0][2] = s; r.m[2][0] = -s; r.m[2][2] = c;
    return r;
}
inline Mat4 rotateZ(float rad) {
    Mat4 r = Mat4::identity();
    const float c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] = c; r.m[0][1] = -s; r.m[1][0] = s; r.m[1][1] = c;
    return r;
}

inline float radians(float deg) { return deg * kPi / 180.0f; }
