#pragma once
// PHAN A — TRINH SOAN DUONG CONG (Khang, DHMTPTB3-16, -20, -34, -40).
// Phan logic (khong co giao dien) de kiem tra duoc bang `make check`; giao dien o editor_view.
//
// Toa do: mat phang profile (x = ban kinh >= 0, y = do cao), don vi met, z = 0.
#include <string>
#include <vector>

#include "mesh.hpp"
#include "modeling.hpp"

struct CurveEditor {
    enum class Kind { Bezier, BSpline };
    Kind kind = Kind::Bezier;
    std::vector<Vec3> points;  // diem dieu khien theo thu tu

    // ---- Bezier nhieu doan (DHMTPTB3-20) ----
    // Diem 0, 3, 6, ... la DIEM NOI (nam tren duong cong); 2 diem giua moi doan la diem dieu khien.
    // Doan k dung points[3k .. 3k+3]. Diem du o cuoi (chua du 1 doan) van hien nhung chua ve cong.
    // Lien tuc C1 tai diem noi J = P[3k]:  P[3k+1] - J = J - P[3k-1]  (2 tiep tuyen bang nhau).
    //
    // ---- B-spline bac 3 deu (DHMTPTB3-34) ----
    // Doan i dung 4 diem lien tiep; doi sang Bezier bang ma tran co so:
    //   B0 = (P0 + 4P1 + P2) / 6   B1 = (2P1 + P2) / 3   B2 = (P1 + 2P2) / 3   B3 = (P1 + 4P2 + P3) / 6
    // Tu dong lien tuc C2; keo 1 diem chi doi 4 doan quanh no (dieu khien cuc bo).
    // Diem dau, cuoi duoc lap 3 lan de duong cong bat dau / ket thuc dung tai do (cham truc quay).

    int segmentCount() const;   // so doan cong (sau khi doi sang Bezier)
    Profile toProfile() const;  // chuoi doan Bezier de dua vao revolve()

    // Them diem o cuoi. Bezier: khi bat dau doan moi (sau mot diem noi), diem dieu khien dau tien
    // duoc them TU DONG doi xung qua diem noi de giu C1 -> moi doan sau chi can click 2 diem.
    // Tra ve chi so diem vua click.
    int addPoint(Vec3 p);
    // Di chuyen diem i, giu rang buoc (Bezier: C1 tai diem noi). x bi kep >= 0 (khong vuot truc quay).
    void movePoint(int i, Vec3 p);
    // Xoa: B-spline xoa 1 diem; Bezier xoa ca doan chua diem do (giu cau truc 3n + 1).
    void removePoint(int i);
    // Diem gan (x, y) nhat trong ban kinh r; -1 neu khong co.
    int nearest(Vec3 p, float r) const;
    bool isJoint(int i) const { return kind == Kind::Bezier && i % 3 == 0; }

    // Nap san tu mot Profile (diem dau doan 0, roi P1, P2, P3 cua tung doan).
    void fromProfile(const Profile& profile);

    // Luu / tai diem dieu khien (DHMTPTB3-40). Dinh dang van ban:
    //   # binh luan
    //   kind bezier | bspline
    //   p <x> <y>
    bool save(const std::string& path, std::string& error) const;
    bool load(const std::string& path, std::string& error);
};

// Ghi luoi ra file .obj (v, vt, vn, f v/vt/vn) — mo duoc bang Blender (DHMTPTB3-40).
bool exportObj(const Mesh& mesh, const std::string& path, std::string& error);
