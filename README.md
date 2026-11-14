# Phòng trưng bày 3D

Đồ án cuối kỳ môn Đồ họa máy tính, nhóm 3 người:

- **Lê Duy Khang** (24IT105)
- **Nguyễn Văn Hảo** (24IT053)
- **Nguyễn Hải Lâm** (24IT134)

Chương trình dựng một phòng trưng bày 3D. Các vật thể được sinh ra từ đường cong Bezier và mặt tròn xoay, sau đó được vẽ bằng **bộ vẽ do nhóm tự cài trên CPU**. OpenGL/GLUT chỉ dùng để mở cửa sổ, nhận phím và chuột, và đưa ảnh lên màn hình bằng `glDrawPixels`.

## Build và chạy

| Máy | Cần cài | Lệnh |
| --- | --- | --- |
| macOS | `xcode-select --install` | `make run` |
| Windows | MinGW và [freeglut cho MinGW](https://github.com/TransmissionZero/freeglut-MinGW) | `mingw32-make run` |
| Linux | `sudo apt install freeglut3-dev` | `make run` |

`make check` chạy các bài kiểm tra thuật toán mà không cần mở cửa sổ.

Bục giữa phòng mặc định đặt ấm trà Utah ([assets/teapot.obj](assets/teapot.obj)). Muốn nạp vật khác thì chạy `./build/phong_trung_bay duong/dan/vat.obj`.

### Demo hiển thị của Lâm (DHMTPTB3-18, DHMTPTB3-19)

Sau `make`, chạy `./build/phong_trung_bay --render-demo` (Windows:
`build\phong_trung_bay.exe --render-demo`) để xem hai tam giác màu phẳng
không phụ thuộc vào Mesh/camera chưa hoàn thiện. Phím `T` chuyển demo/cảnh.

- Framebuffer RGBA giữ cố định **960×540**, đưa lên màn hình bằng `glDrawPixels`
  và `glPixelZoom`. Resize chỉ đổi viewport/tỷ lệ hiển thị, không đổi độ phân
  giải render. Tọa độ logic có gốc trên trái, dữ liệu ảnh lưu hàng dưới trước.
- `glutIdleFunc` yêu cầu vẽ lại liên tục, không còn timer giới hạn 16 ms. FPS
  đo frame hoàn tất theo khoảng tối thiểu 0,5 giây; vẫn có thể bị giới hạn bởi
  vsync/driver. Khi so sánh CPU renderer, dùng thêm thời gian biến đổi/tô (ms)
  trên HUD, giữ cùng cảnh, chế độ và độ phân giải 960×540.
- Scanline sắp đỉnh theo y, chia tại đỉnh giữa thành tam giác đáy bằng và
  đỉnh bằng. Mỗi nửa tính hai hệ số `dx/dy = 1/m` một lần, rồi cộng dồn
  `xLeft += dxLeft/dy`, `xRight += dxRight/dy` qua từng dòng. Không còn tìm
  giao cả ba cạnh ở mỗi dòng. Lấy mẫu tâm pixel, dùng biên nửa mở
  `[xLeft, xRight)` và `[yMin, yMax)`; bỏ nửa có chiều cao 0, tam giác suy
  biến và tọa độ không hữu hạn; kẹp phạm vi trước khi đổi sang chỉ số nguyên.
- Hiện chỉ tô màu phẳng `tri[0].color`. Phím 3/4 tạm dùng **flat fallback**
  (HUD ghi rõ), không còn bỏ vẽ; chưa phải Gouraud/Phong thật. Phong fallback
  được khởi tạo từ màu vật liệu. Lighting, Z-buffer, Painter và Back-face
  vẫn thuộc task sau. Demo luôn dùng màu phẳng, độc lập các chế độ 3D.
- Kiểm tra thủ công: hai tam giác cam/xanh không lật dọc; FPS cập nhật khi
  đứng yên; resize cửa sổ vẫn giữ hình đúng tỷ lệ tương đối; `T` chuyển cảnh
  và `Esc` thoát. `make check` kiểm tra RGBA, depth reset, FPS bằng thời gian
  giả lập, scanline, cạnh chung, hoán vị đỉnh, clipping màn hình và đầu vào lỗi.

## Phím tắt

| Phím | Chức năng |
| --- | --- |
| 1 / 2 / 3 / 4 | Khung dây / Flat / Gouraud / Phong |
| L | Phong ↔ Blinn-Phong |
| B | Bật/tắt Back-face |
| Z | Bật/tắt Z-buffer |
| O | Painter (thay cho Z-buffer) |
| V | Hiện mảng độ sâu |
| P | Chiếu phối cảnh ↔ song song |
| C | Camera đi bộ ↔ orbit |
| W A S D, mũi tên | Di chuyển |
| Kéo chuột trái | Xoay góc nhìn |
| Click chuột trái | Chọn vật bằng tia: orbit quanh vật, hiện tên, số tam giác, vật liệu |
| + / -, con lăn chuột | Phóng to / thu nhỏ (orbit); khi đi bộ con lăn để đi tới/lùi |
| G | Khung dây phủ bằng GL_LINES (dùng cùng phím X để xem phần bị xén mặt gần) |
| X | Debug xén mặt gần: tô màu cam phần tam giác bị cắt, đỏ là cạnh nằm trên mặt phẳng gần |
| K | Cảnh quả cầu xuyên tấm phẳng (bấm O để thấy Painter vẽ sai) ↔ phòng trưng bày |
| H | Bật/tắt bóng đổ phẳng xuống sàn |
| Kéo quả cầu đèn | Dời đèn trên mặt phẳng ngang (có 2 đèn: ấm bên trái, lạnh bên phải) |
| F | Mức tối ưu FPS: 1 luồng → 4 luồng → 4 luồng + nửa độ phân giải khi di chuyển (mặc định) |
| U | Bật/tắt texture (sàn gỗ, bục đá) |
| I | Nội suy texture đúng phối cảnh ↔ tuyến tính trên màn hình (để thấy sàn bị méo khi nội suy sai) |
| Esc | Thoát |
| T | Demo scanline màu phẳng ↔ cảnh 3D |

## Ai làm phần nào

| Phần | Người | Thư mục | Thuật toán |
| --- | --- | --- | --- |
| A · Mô hình hóa | Khang | `src/modeling/` | Bezier (de Casteljau), B-spline, mặt tròn xoay, pháp tuyến đỉnh, Bresenham |
| B · Cảnh và camera | Hảo | `src/scene/` | look-at, chiếu song song/phối cảnh, xén mặt gần (Sutherland–Hodgman), chọn vật bằng tia |
| C · Hiển thị | Lâm | `src/render/` | tô dòng quét, Phong, Flat/Gouraud/Phong, Back-face, Painter, Z-buffer |
| Dùng chung | cả nhóm | `include/` | `math3d.hpp`, `mesh.hpp`, `framebuffer.hpp`, `pipeline.hpp` |

Chỗ cần viết thuật toán được đánh dấu `TODO(<tên>, Sprint n)`. Tìm phần của mình bằng lệnh `grep -rn "TODO(Khang" src`.

**Không sửa file trong `include/` khi chưa báo cả nhóm.** Đó là "hợp đồng" giữa ba phần.

## Quy trình Git

1. Mỗi task Jira (project **DHMTPTB3**, https://nguyenvanhaone.atlassian.net) làm trên **một nhánh riêng**, đặt tên theo mã task: `feature/DHMTPTB3-17-bezier`.
2. Commit nhỏ, ghi mã task ở đầu: `DHMTPTB3-17: de Casteljau cho Bezier bac 3`. Jira sẽ tự gắn commit vào task.
3. Trước khi mở Pull Request: `git pull origin main`, rồi chạy `make` và `make check` không lỗi.
4. Mở Pull Request vào `main`. Cần **1 người khác duyệt** trước khi merge.
5. Không push thẳng lên `main`.

```bash
git checkout main && git pull
git checkout -b feature/DHMTPTB3-17-bezier
# ... code ...
git add -A && git commit -m "DHMTPTB3-17: de Casteljau cho Bezier bac 3"
git push -u origin feature/DHMTPTB3-17-bezier
```

## Cấu trúc

```
include/        math3d, mesh, framebuffer, pipeline, texture (dùng chung)
src/main.cpp    cửa sổ GLUT, phím tắt, hiện FPS
src/modeling/   phần A
src/scene/      phần B (camera, phòng, xén, chọn vật, nạp .obj, nạp texture)
src/third_party/ stb_image.h (nạp ảnh, public domain)
src/render/     phần C (renderScene đã nối sẵn các bước)
src/platform/   chọn header GLUT theo hệ điều hành
assets/         mô hình .obj mẫu (ấm trà Utah), ảnh texture
tools/          gen_textures.py: tạo ảnh texture
tests/          make check
```
