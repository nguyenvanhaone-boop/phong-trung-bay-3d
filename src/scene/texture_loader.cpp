// PHAN B — NAP ANH TEXTURE (Hao, Sprint 5, DHMTPTB3-41).
#include "texture_loader.hpp"

#include <algorithm>

// stb_image: thu vien mot file, public domain. Chi bat phan cai dat o DUNG MOT file .cpp.
// Tat canh bao cua thu vien ngoai de `make` van sach voi -Wall -Wextra.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#  pragma clang diagnostic ignored "-Wunused-but-set-variable"
#  pragma clang diagnostic ignored "-Wtype-limits"
#  pragma clang diagnostic ignored "-Wunused-function"
#  pragma clang diagnostic ignored "-Wmissing-field-initializers"
#  pragma clang diagnostic ignored "-Wsign-compare"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#  pragma GCC diagnostic ignored "-Wtype-limits"
#  pragma GCC diagnostic ignored "-Wunused-function"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_FAILURE_USERMSG
#include "third_party/stb_image.h"
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

std::shared_ptr<const Texture> loadTexture(const std::string& path, std::string& error) {
    int w = 0, h = 0, channels = 0;
    // Yeu cau 3 kenh: anh xam / co alpha deu duoc stb doi ve RGB. Hang 0 = hang tren cung.
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 3);
    if (!data) {
        const char* why = stbi_failure_reason();
        error = path + ": " + (why ? why : "khong doc duoc anh");
        return nullptr;
    }
    auto tex = std::make_shared<Texture>();
    tex->width = w;
    tex->height = h;
    tex->texels.resize(static_cast<std::size_t>(w) * h);
    for (std::size_t i = 0; i < tex->texels.size(); ++i)
        tex->texels[i] = Vec3{static_cast<float>(data[3 * i]), static_cast<float>(data[3 * i + 1]),
                              static_cast<float>(data[3 * i + 2])} / 255.0f;
    stbi_image_free(data);
    return tex;
}

std::shared_ptr<const Texture> makeCheckerTexture(int size, int cells, Vec3 a, Vec3 b) {
    auto tex = std::make_shared<Texture>();
    tex->width = tex->height = size;
    tex->texels.resize(static_cast<std::size_t>(size) * size);
    const int cell = std::max(1, size / std::max(1, cells));
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
            tex->texels[static_cast<std::size_t>(y) * size + x] = ((x / cell + y / cell) % 2 == 0) ? a : b;
    return tex;
}
