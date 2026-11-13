# Mô hình mẫu

| File | Nguồn | Ghi chú |
| --- | --- | --- |
| `teapot.obj` | Ấm trà Utah (Martin Newell, 1975), bản lưới tam giác lấy từ [alecjacobson/common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models) | 3644 đỉnh, 6320 tam giác, chỉ có `v` và `f` (pháp tuyến do bộ đọc tự tính) |

Muốn thử vật khác: `./build/phong_trung_bay duong/dan/vat.obj`. Vật được thu về cao 0,45 m và đặt lên bục giữa phòng.

## Texture

| File | Nguồn | Dùng cho |
| --- | --- | --- |
| `textures/san_go.png` | Nhóm tự tạo bằng `python3 tools/gen_textures.py` | Sàn: 5 tấm ván gỗ, 256 × 256 ứng với 1 m × 1 m |
| `textures/da_cam_thach.png` | Nhóm tự tạo bằng `python3 tools/gen_textures.py` | Các bục: đá cẩm thạch xám |

Ảnh được nạp bằng [stb_image](https://github.com/nothings/stb) v2.32 (public domain, nằm ở `src/third_party/stb_image.h`). Ảnh lặp liền mạch, vì hình hộp đặt (u, v) theo mét nên mỗi 1 m ảnh lặp lại một lần.
