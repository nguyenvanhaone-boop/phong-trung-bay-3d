#pragma once
// PHAN B — NAP ANH TEXTURE (Hao, Sprint 5, DHMTPTB3-41). Dung stb_image (src/third_party).
#include <memory>
#include <string>

#include "texture.hpp"

// Nap anh PNG/JPG/BMP/TGA thanh Texture (RGB, [0, 1]). Loi thi tra ve null va ghi `error`.
std::shared_ptr<const Texture> loadTexture(const std::string& path, std::string& error);

// Anh o co 2 mau, n x n o (dung khi khong co file anh, va trong test).
std::shared_ptr<const Texture> makeCheckerTexture(int size, int cells, Vec3 a, Vec3 b);
