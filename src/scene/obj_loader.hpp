#pragma once
// PHAN B — NAP FILE .OBJ (Hao, Sprint 5, DHMTPTB3-42).
//
// Dinh dang Wavefront .obj (van ban, moi dong mot lenh):
//   v  x y z        vi tri dinh
//   vt u v          toa do texture
//   vn x y z        phap tuyen
//   f  a b c ...    mat (>= 3 dinh), moi dinh viet mot trong 4 kieu:
//                   v   v/vt   v//vn   v/vt/vn   (chi so bat dau tu 1, so am = dem tu cuoi len)
// Cac lenh khac (o, g, s, mtllib, usemtl, l, ...) va dong chu thich '#' duoc bo qua.
#include <istream>
#include <string>

#include "mesh.hpp"

// Doc .obj tu luong van ban. Tra ve false va ghi `error` (kem so dong) neu file sai.
// Ket qua dung quy uoc mesh.hpp: tam giac, phap tuyen dinh da chuan hoa.
//   - Mat nhieu canh duoc chia thanh quat tam giac (0, i, i+1).
//   - Trong .obj moi dinh cua mat co 3 chi so rieng (v, vt, vn); Mesh chi co 1 chi so,
//     nen moi bo (v, vt, vn) khac nhau thanh mot Vertex rieng (bo trung thi dung lai).
//   - File khong co vn: tinh phap tuyen dinh = trung binh phap tuyen cac mat ke (theo goc tai dinh).
bool parseObj(std::istream& in, Mesh& out, std::string& error);

// Doc .obj tu file. Ten luoi = ten file (bo duong dan).
bool loadObj(const std::string& path, Mesh& out, std::string& error);

// Dua luoi ve dung kich thuoc de dat len buc: cao `height` met, day nam tren y = 0,
// tam theo x, z o goc. Doi truc tiep toa do dinh (khong dung ma tran ti le) nen
// ma tran dat vat van chi co tinh tien + quay va phap tuyen khong bi lech.
void fitToHeight(Mesh& mesh, float height);
