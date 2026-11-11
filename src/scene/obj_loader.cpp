// PHAN B — NAP FILE .OBJ (Hao, Sprint 5, DHMTPTB3-42).
#include "obj_loader.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

namespace {

// Doi chi so .obj (bat dau tu 1, so am = tinh tu cuoi) sang chi so mang (bat dau tu 0).
// count = so phan tu da doc toi dong nay. Tra ve -1 neu sai.
int resolveIndex(long raw, std::size_t count) {
    const long n = static_cast<long>(count);
    const long i = raw > 0 ? raw - 1 : n + raw;  // raw = -1 la phan tu vua doc
    return (raw != 0 && i >= 0 && i < n) ? static_cast<int>(i) : -1;
}

bool parseLong(const std::string& s, long& value) {
    if (s.empty()) return false;
    char* end = nullptr;
    errno = 0;
    value = std::strtol(s.c_str(), &end, 10);
    return errno == 0 && end && *end == '\0';
}

}  // namespace

bool parseObj(std::istream& in, Mesh& out, std::string& error) {
    std::vector<Vec3> positions, normals;
    std::vector<std::array<float, 2>> texcoords;

    out.vertices.clear();
    out.triangles.clear();
    std::map<std::array<int, 3>, int> unified;  // bo (v, vt, vn) -> chi so Vertex trong Mesh
    std::vector<int> positionOf;                // Vertex i lay vi tri tu positions[positionOf[i]]
    std::vector<bool> hasNormal;

    std::string line;
    int lineNo = 0;
    auto fail = [&](const std::string& what) {
        error = "dong " + std::to_string(lineNo) + ": " + what;
        return false;
    };

    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();  // file tao tren Windows
        std::istringstream ls(line);
        std::string cmd;
        if (!(ls >> cmd) || cmd[0] == '#') continue;

        if (cmd == "v" || cmd == "vn") {
            Vec3 p;
            if (!(ls >> p.x >> p.y >> p.z)) return fail("can 3 so sau '" + cmd + "'");
            if (cmd == "v") positions.push_back(p);
            else normals.push_back(normalize(p));
        } else if (cmd == "vt") {
            std::array<float, 2> t{0, 0};
            if (!(ls >> t[0])) return fail("can it nhat 1 so sau 'vt'");
            ls >> t[1];  // v co the thieu (texture 1 chieu)
            texcoords.push_back(t);
        } else if (cmd == "f") {
            std::vector<int> face;  // chi so Vertex (da gop) cua cac dinh cua mat
            std::string ref;
            while (ls >> ref) {
                // Tach "a/b/c": phan rong (vd "a//c") nghia la khong co thuoc tinh do.
                std::array<std::string, 3> part;
                int k = 0;
                for (char ch : ref) {
                    if (ch == '/') { if (++k > 2) return fail("dinh '" + ref + "' co qua 3 phan"); }
                    else part[k] += ch;
                }
                std::array<int, 3> key{-1, -1, -1};
                const std::size_t counts[3] = {positions.size(), texcoords.size(), normals.size()};
                const char* names[3] = {"v", "vt", "vn"};
                for (int j = 0; j < 3; ++j) {
                    if (part[j].empty()) {
                        if (j == 0) return fail("dinh '" + ref + "' thieu chi so v");
                        continue;
                    }
                    long raw;
                    if (!parseLong(part[j], raw)) return fail("chi so '" + part[j] + "' khong phai so nguyen");
                    key[j] = resolveIndex(raw, counts[j]);
                    if (key[j] < 0) return fail(std::string("chi so ") + names[j] + " = " + part[j] + " ngoai pham vi");
                }

                auto it = unified.find(key);
                if (it == unified.end()) {
                    Vertex v;
                    v.position = positions[key[0]];
                    if (key[1] >= 0) { v.u = texcoords[key[1]][0]; v.v = texcoords[key[1]][1]; }
                    if (key[2] >= 0) v.normal = normals[key[2]];
                    it = unified.emplace(key, static_cast<int>(out.vertices.size())).first;
                    out.vertices.push_back(v);
                    positionOf.push_back(key[0]);
                    hasNormal.push_back(key[2] >= 0);
                }
                face.push_back(it->second);
            }
            if (face.size() < 3) return fail("mat can it nhat 3 dinh");
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {  // quat tam giac
                const std::array<int, 3> t{face[0], face[i], face[i + 1]};
                if (t[0] == t[1] || t[1] == t[2] || t[0] == t[2]) continue;  // tam giac suy bien
                out.triangles.push_back(t);
            }
        }
        // Lenh khac: bo qua.
    }

    if (out.triangles.empty()) {
        error = "file khong co mat nao (lenh f)";
        return false;
    }

    // Phap tuyen cho dinh khong co vn: cong phap tuyen (don vi) cac mat ke, nhan voi GOC cua mat
    // tai dinh do. Trong so theo goc khong phu thuoc cach chia da giac thanh tam giac (trong so
    // theo dien tich thi co: mot mat vuong chia 2 tam giac se "keo" phap tuyen ve phia duong cheo).
    // Cong theo VI TRI (khong theo Vertex), nen hai Vertex cung vi tri nhung khac vt (duong may
    // texture) van cho cung mot phap tuyen.
    if (std::find(hasNormal.begin(), hasNormal.end(), false) != hasNormal.end()) {
        std::vector<Vec3> acc(positions.size());
        for (const auto& t : out.triangles) {
            const Vec3 n = normalize(faceNormal(out, t));
            for (int k = 0; k < 3; ++k) {
                const Vec3 p = out.vertices[t[k]].position;
                const Vec3 e1 = normalize(out.vertices[t[(k + 1) % 3]].position - p);
                const Vec3 e2 = normalize(out.vertices[t[(k + 2) % 3]].position - p);
                const float angle = std::acos(std::clamp(dot(e1, e2), -1.0f, 1.0f));
                acc[positionOf[t[k]]] = acc[positionOf[t[k]]] + n * angle;
            }
        }
        for (std::size_t i = 0; i < out.vertices.size(); ++i) {
            if (hasNormal[i]) continue;
            const Vec3 n = acc[positionOf[i]];
            out.vertices[i].normal = length(n) > 0 ? normalize(n) : Vec3{0, 1, 0};
        }
    }
    return true;
}

bool loadObj(const std::string& path, Mesh& out, std::string& error) {
    std::ifstream file(path);
    if (!file) {
        error = "khong mo duoc file " + path;
        return false;
    }
    if (!parseObj(file, out, error)) {
        error = path + ", " + error;
        return false;
    }
    std::string name = path.substr(path.find_last_of("/\\") + 1);
    const std::size_t dot = name.find_last_of('.');
    if (dot != std::string::npos && dot > 0) name.resize(dot);
    out.name = name;
    return true;
}

void fitToHeight(Mesh& mesh, float height) {
    if (mesh.vertices.empty()) return;
    Vec3 lo = mesh.vertices[0].position, hi = lo;
    for (const Vertex& v : mesh.vertices) {
        const Vec3 p = v.position;
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }
    const float h = hi.y - lo.y;
    const float s = h > 0 ? height / h : 1.0f;  // ti le deu: phap tuyen giu nguyen huong
    const Vec3 base{(lo.x + hi.x) * 0.5f, lo.y, (lo.z + hi.z) * 0.5f};
    for (Vertex& v : mesh.vertices) v.position = (v.position - base) * s;
}
