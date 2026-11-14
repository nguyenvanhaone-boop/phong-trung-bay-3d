#pragma once
// PHAN A — MO HINH HOA (Khang). Slide CG.5, Lab 05.
#include <array>
#include <vector>

#include "mesh.hpp"

// Diem tren duong cong Bezier bac 3 tai t in [0, 1], tinh bang de Casteljau.
Vec3 bezierPoint(const std::array<Vec3, 4>& P, float t);

// Dao ham B'(t) (huong tiep tuyen), dung de tinh phap tuyen giai tich.
Vec3 bezierTangent(const std::array<Vec3, 4>& P, float t);

// Cac diem trung gian cua de Casteljau tai t (3 + 2 + 1 diem), de ve minh hoa khi thuyet trinh.
std::vector<Vec3> deCasteljauLevels(const std::array<Vec3, 4>& P, float t);

// Mot profile la nhieu doan Bezier noi nhau: diem cuoi doan i = diem dau doan i+1.
struct Profile {
    std::vector<std::array<Vec3, 4>> segments;  // toa do trong mat phang xy, x >= 0 (ban kinh)
};

// Quay profile quanh truc y: rings mau tren moi doan, slices lat quanh truc.
// Ket qua: Mesh co phap tuyen dinh va (u, v), dung quy uoc trong mesh.hpp.
Mesh revolve(const Profile& profile, int ringsPerSegment, int slices, const Material& material);

// Doan thang a -> b viet duoi dang Bezier bac 3 (2 diem dieu khien chia 3 doan deu).
std::array<Vec3, 4> bezierLine(Vec3 a, Vec3 b);

// Mat co san (Sprint 4): deu la mat tron xoay voi profile don gian.
Mesh makeSphere(float radius, int rings, int slices, const Material& material);          // tam o goc
Mesh makeCylinder(float radius, float height, int slices, const Material& material);     // day o y = 0
Mesh makeCone(float radius, float height, int slices, const Material& material);         // day o y = 0
Mesh makeTorus(float majorRadius, float minorRadius, int rings, int slices, const Material& material);  // tam o goc

// Profile cua 5 vat trung bay (binh, binh co cao, qua cau, bat, lo), day vat o y = 0, don vi met.
Profile exhibitProfile(int index);
