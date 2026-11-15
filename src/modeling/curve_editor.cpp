// PHAN A — TRINH SOAN DUONG CONG (Khang, DHMTPTB3-16, -20, -34, -40).
#include "curve_editor.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {

Vec3 clampToAxis(Vec3 p) { return {std::max(0.0f, p.x), p.y, 0}; }

}  // namespace

int CurveEditor::segmentCount() const {
    const int n = static_cast<int>(points.size());
    if (kind == Kind::Bezier) return n >= 4 ? (n - 1) / 3 : 0;
    return n >= 2 ? n + 1 : 0;  // n diem + lap 2 lan moi dau -> (n + 4) - 3 doan
}

Profile CurveEditor::toProfile() const {
    Profile pr;
    const int n = static_cast<int>(points.size());
    if (kind == Kind::Bezier) {
        for (int k = 0; 3 * k + 3 < n; ++k)
            pr.segments.push_back({points[3 * k], points[3 * k + 1], points[3 * k + 2], points[3 * k + 3]});
        return pr;
    }
    if (n < 2) return pr;
    // B-spline: lap diem dau va cuoi them 2 lan -> duong cong di qua dung 2 dau mut.
    std::vector<Vec3> q;
    q.push_back(points.front());
    q.push_back(points.front());
    q.insert(q.end(), points.begin(), points.end());
    q.push_back(points.back());
    q.push_back(points.back());
    for (std::size_t i = 0; i + 3 < q.size(); ++i) {
        const Vec3 P0 = q[i], P1 = q[i + 1], P2 = q[i + 2], P3 = q[i + 3];
        pr.segments.push_back({(P0 + P1 * 4 + P2) / 6.0f, (P1 * 2 + P2) / 3.0f, (P1 + P2 * 2) / 3.0f, (P1 + P2 * 4 + P3) / 6.0f});
    }
    return pr;
}

int CurveEditor::addPoint(Vec3 p) {
    p = clampToAxis(p);
    const int n = static_cast<int>(points.size());
    // Bat dau doan thu 2 tro di (n = 3k + 1, k >= 1): chen diem dieu khien doi xung truoc.
    if (kind == Kind::Bezier && n >= 4 && (n - 1) % 3 == 0) {
        const Vec3 joint = points[n - 1], prev = points[n - 2];
        points.push_back(clampToAxis(joint * 2.0f - prev));
    }
    points.push_back(p);
    return static_cast<int>(points.size()) - 1;
}

void CurveEditor::movePoint(int i, Vec3 p) {
    const int n = static_cast<int>(points.size());
    if (i < 0 || i >= n) return;
    p = clampToAxis(p);
    if (kind == Kind::BSpline) { points[i] = p; return; }

    if (i % 3 == 0) {
        // Keo DIEM NOI: 2 diem dieu khien ben canh tinh tien theo -> van thang hang, giu C1.
        const Vec3 d = p - points[i];
        points[i] = p;
        if (i - 1 >= 0) points[i - 1] = clampToAxis(points[i - 1] + d);
        if (i + 1 < n) points[i + 1] = clampToAxis(points[i + 1] + d);
        return;
    }
    points[i] = p;
    // Keo DIEM DIEU KHIEN: diem doi dien qua diem noi gan nhat di theo (doi xung) -> C1.
    const int joint = (i % 3 == 1) ? i - 1 : i + 1;
    const int other = (i % 3 == 1) ? i - 2 : i + 2;
    if (joint > 0 && joint < n - 1 && other >= 0 && other < n)
        points[other] = clampToAxis(points[joint] * 2.0f - p);
}

void CurveEditor::removePoint(int i) {
    const int n = static_cast<int>(points.size());
    if (i < 0 || i >= n) return;
    if (kind == Kind::BSpline || n <= 4) { points.erase(points.begin() + i); return; }
    // Bezier: xoa ca doan chua diem i (3 diem) de cac diem noi van o vi tri 0, 3, 6, ...
    int seg = std::max(0, (i - 1) / 3);                     // doan chua diem i (diem noi i tinh cho doan truoc)
    if (i == 0) seg = 0;
    const int last = (n - 1) / 3 - 1;                       // doan day du cuoi cung
    if (seg > last) {                                        // diem du o cuoi: xoa rieng no
        points.erase(points.begin() + i);
        return;
    }
    if (i == 0) points.erase(points.begin(), points.begin() + 3);           // bo doan dau (giu P3 lam diem dau)
    else points.erase(points.begin() + 3 * seg + 1, points.begin() + 3 * seg + 4);  // bo P1, P2, P3 cua doan
    // Noi lai: giu C1 tai diem noi vua ghep (neu con 2 phia).
    const int j = (i == 0) ? 0 : 3 * seg;
    if (j > 0 && j + 1 < static_cast<int>(points.size()))
        points[j + 1] = clampToAxis(points[j] * 2.0f - points[j - 1]);
}

int CurveEditor::nearest(Vec3 p, float r) const {
    int best = -1;
    float bestD = r;
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        const float d = length(Vec3{points[i].x - p.x, points[i].y - p.y, 0});
        if (d <= bestD) { bestD = d; best = i; }
    }
    return best;
}

void CurveEditor::fromProfile(const Profile& profile) {
    points.clear();
    kind = Kind::Bezier;
    for (const auto& s : profile.segments) {
        if (points.empty()) points.push_back(s[0]);
        points.push_back(s[1]);
        points.push_back(s[2]);
        points.push_back(s[3]);
    }
}

bool CurveEditor::save(const std::string& path, std::string& error) const {
    std::ofstream f(path);
    if (!f) { error = "khong ghi duoc " + path; return false; }
    f << "# profile mat tron xoay - phong trung bay 3D (x = ban kinh, y = do cao, met)\n";
    f << "kind " << (kind == Kind::Bezier ? "bezier" : "bspline") << "\n";
    for (const Vec3& p : points) f << "p " << p.x << " " << p.y << "\n";
    return static_cast<bool>(f);
}

bool CurveEditor::load(const std::string& path, std::string& error) {
    std::ifstream f(path);
    if (!f) { error = "khong mo duoc " + path; return false; }
    CurveEditor tmp;
    std::string line;
    int lineNo = 0;
    while (std::getline(f, line)) {
        ++lineNo;
        std::istringstream ls(line);
        std::string cmd;
        if (!(ls >> cmd) || cmd[0] == '#') continue;
        if (cmd == "kind") {
            std::string k;
            ls >> k;
            if (k == "bezier") tmp.kind = Kind::Bezier;
            else if (k == "bspline") tmp.kind = Kind::BSpline;
            else { error = "dong " + std::to_string(lineNo) + ": kind phai la bezier hoac bspline"; return false; }
        } else if (cmd == "p") {
            Vec3 p;
            if (!(ls >> p.x >> p.y)) { error = "dong " + std::to_string(lineNo) + ": can 2 so sau 'p'"; return false; }
            tmp.points.push_back(clampToAxis(p));
        } else {
            error = "dong " + std::to_string(lineNo) + ": lenh '" + cmd + "' khong hieu";
            return false;
        }
    }
    *this = tmp;
    return true;
}

bool exportObj(const Mesh& mesh, const std::string& path, std::string& error) {
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) { error = "khong ghi duoc " + path; return false; }
    std::fprintf(f, "# %s - xuat tu phong trung bay 3D\n", mesh.name.c_str());
    std::fprintf(f, "o %s\n", mesh.name.empty() ? "vat" : mesh.name.c_str());
    for (const Vertex& v : mesh.vertices) std::fprintf(f, "v %.6f %.6f %.6f\n", v.position.x, v.position.y, v.position.z);
    for (const Vertex& v : mesh.vertices) std::fprintf(f, "vt %.6f %.6f\n", v.u, v.v);
    for (const Vertex& v : mesh.vertices) std::fprintf(f, "vn %.6f %.6f %.6f\n", v.normal.x, v.normal.y, v.normal.z);
    // Moi Vertex co du v, vt, vn cung chi so -> f a/a/a (chi so .obj bat dau tu 1).
    for (const auto& t : mesh.triangles)
        std::fprintf(f, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", t[0] + 1, t[0] + 1, t[0] + 1, t[1] + 1, t[1] + 1, t[1] + 1, t[2] + 1, t[2] + 1, t[2] + 1);
    const bool ok = std::ferror(f) == 0;
    std::fclose(f);
    if (!ok) error = "loi khi ghi " + path;
    return ok;
}
