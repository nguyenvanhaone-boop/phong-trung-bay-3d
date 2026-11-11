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
| G | Khung dây tạm bằng GL_LINES (chỉ để kiểm tra camera khi phần C chưa xong) |
| X | Debug xén mặt gần: tô màu cam phần tam giác bị cắt, đỏ là cạnh nằm trên mặt phẳng gần |
| Esc | Thoát |

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
include/        math3d, mesh, framebuffer, pipeline (dùng chung)
src/main.cpp    cửa sổ GLUT, phím tắt, hiện FPS
src/modeling/   phần A
src/scene/      phần B
src/render/     phần C (renderScene đã nối sẵn các bước)
src/platform/   chọn header GLUT theo hệ điều hành
tests/          make check
```
