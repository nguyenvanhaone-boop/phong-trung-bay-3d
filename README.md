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
| W A S D, mũi tên ↑ ↓ (giữ phím) | Nhân vật đi theo hướng camera; ← → quay camera |
| Tab | Góc nhìn thứ 3 (camera bay sau nhân vật, không xuyên tường) ↔ góc nhìn thứ nhất |
| C | Xem vật / tranh gần điểm đang nhìn (orbit) ↔ quay lại đi bộ |
| Kéo chuột trái | Xoay góc nhìn |
| Click chuột trái | Chọn vật bằng tia (xuyên qua kính): orbit quanh vật, hiện tên tranh / cổ vật, số tam giác, vật liệu |
| + / -, con lăn chuột | Camera xa / gần nhân vật; khi đang xem vật thì zoom |
| G | Khung dây phủ bằng GL_LINES (dùng cùng phím X để xem phần bị xén mặt gần) |
| X | Debug xén mặt gần: tô màu cam phần tam giác bị cắt, đỏ là cạnh nằm trên mặt phẳng gần |
| K | Cảnh quả cầu xuyên tấm phẳng (bấm O để thấy Painter vẽ sai) ↔ phòng trưng bày |
| H | Bật/tắt bóng đổ phẳng xuống sàn |
| J | Bật/tắt khách tham quan tự đi |
| Kéo quả cầu đèn | Dời đèn trên mặt phẳng ngang, trong khu của đèn (mỗi khu có đèn riêng) |
| F | Mức tối ưu FPS: 1 luồng → 4 luồng → 4 luồng + nửa độ phân giải khi di chuyển (mặc định) |
| E | Mở / đóng trình soạn đường cong (xem bảng dưới) |
| N | Hiện pháp tuyến đỉnh dạng gạch ngắn |
| U | Bật/tắt texture (sàn gỗ, bục đá) |
| I | Nội suy texture đúng phối cảnh ↔ tuyến tính trên màn hình (để thấy sàn bị méo khi nội suy sai) |
| Esc | Thoát |
| T | Demo scanline màu phẳng ↔ cảnh 3D |

### Bảo tàng mở rộng

```
 z=-6 +-----------+  hành lang A: tranh Đông Hồ, Hokusai   +-------------+
      |           |=========================================|             |
      |   SẢNH    |                                         |  PHÒNG CỔ   |
      |   CHÍNH   |                                         |  VẬT (7 tủ  |
      |           |=========================================|  kính)      |
 z=6  +-----------+  hành lang B: tranh châu Âu             +-------------+
     x=-6        x=6                                       x=18          x=28
```

- **Sân trước ngoài trời** (x −5..5, z 10..15): nhân vật bắt đầu ở đây.
  - Mặt tiền ốp đá cao bằng nhà, tường rào thấp 1,1 m, nền lát đá, 4 chậu cây.
  - Không có trần: nền khung hình là màu trời. Một "mặt trời" ở rất xa, không suy giảm, nên bóng đổ gần như song song.
- **Sảnh đón** (x −2..2, z 6..10): cửa chính gỗ lim 2 cánh **luôn mở**, áp sát hai tường bên. Hai cánh cửa là vật cản mỏng.
- **Người ra, người vào:**
  - Khách mới xuất hiện ở mép sân, cứ 4–9 giây một người, tối đa 7 người cùng lúc.
  - Mỗi người đi qua cửa chính và cổng vào, tham quan theo 1 trong 3 kiểu: chỉ sảnh, sảnh + hành lang A, hoặc cả vòng.
  - Xong thì đi ngược ra theo làn bên kia (không đâm đầu người đang vào) và biến mất ở mép sân.
- **Khung cửa tự sinh:** chỗ hai khu giáp nhau có 2 cột, dầm ngang và chỉ vàng (cổng chính và 4 cửa hành lang). Cột là vật cản.
- **Trang trí tường:** giấy dán tường hoa văn (`hoa_van_tuong.png` xám nhạt, nhân với màu từng khu), ốp gỗ chân tường cao 0,9 m, nẹp chân, nẹp ngang, phào trần.
- **Đèn thả trần** (13 đèn): dây treo, chao đồng 2 mặt (lòng chao sáng), bóng phát sáng. Ánh sáng suy giảm theo khoảng cách, I × 1 / (1 + k·d²) (`Light::attenuation`), nên mỗi đèn sáng một vùng quanh nó. Kéo chao hoặc bóng đèn để dời đèn.
- **Sảnh chính** giữ nguyên phòng cũ: 5 bục mặt tròn xoay và bục giữa nạp `.obj`.
- **Hai hành lang** treo 12 bức tranh public domain (nguồn trong [assets/README.md](assets/README.md)), khung mạ vàng, mỗi hành lang có 2 đèn trần.
- **Phòng cổ vật** có 7 tủ kính: trống đồng, bình gốm hoa lam, lư hương ba chân, chén ngọc, bình hồ lô, thạp đồng, ấn ngọc. Tất cả là mặt tròn xoay dựng từ profile Bezier của phần A, kèm texture hoa văn tự sinh.
- **Tường không vẽ tay.** Mặt bằng là các hình chữ nhật, được chia thành lưới ô 0,25 m. Giữa ô đi được và ô không đi được là một đoạn tường, nên chỗ hai khu giáp nhau tự thành cửa. Lưới này dùng luôn cho va chạm: nhân vật trượt dọc tường, camera góc nhìn thứ 3 co lại trước tường.
- **Nhân vật** dựng từ các khối của phần A (thân là mặt tròn xoay, đầu là cầu, tay chân là trụ). Dáng đi dùng ma trận phân cấp: thân → khớp hông / vai → chân / tay, góc vung theo quãng đường đã đi.
- **Khách tham quan tự đi** (phím J, [src/scene/visitor.cpp](src/scene/visitor.cpp)):
  - Tới tranh hoặc tủ kính thì dừng vài giây và quay mặt vào ngắm.
  - Có người phía trước thì đứng chờ, chờ lâu thì bước sang ngang. Kẹt quá 3 giây thì bỏ qua điểm đó.
  - Nhân vật của bạn không đi xuyên qua khách được.
- **Đèn theo khu.** Mỗi vật chỉ được chiếu bởi đèn trong khu của nó, vì tường chắn sáng. Bóng của đèn i được tô bằng các đèn còn lại, nên vùng bóng của đèn 1 vẫn được đèn 2 chiếu tới.
- **Kính trong suốt** (`Material::opacity < 1`):
  - Vẽ sau mọi vật đục, xếp từ xa đến gần, không ghi Z-buffer.
  - Màu mới = màu cũ × (1 − độ đục) + màu kính.
  - Click chuột xuyên qua kính để chọn cổ vật bên trong.
- **Bỏ vật ngoài khung nhìn:** mỗi vật có mặt cầu bao. Vật nằm ngoài 6 mặt phẳng của hình chóp nhìn (lấy thẳng từ ma trận P·V) bị bỏ trước khi biến đổi, nên đứng ở hành lang thì không phải biến đổi các vật trong sảnh.

### Trình soạn đường cong (phím E)

Nửa trái là mặt phẳng profile (x = bán kính, y = độ cao, lưới 10 cm, trục quay màu đỏ). Nửa phải là mặt tròn xoay sinh ra từ đường cong, xoay chậm, cập nhật ngay khi kéo điểm.

| Phím / chuột | Chức năng |
| --- | --- |
| Chuột trái | Thêm điểm, hoặc kéo điểm có sẵn. Bezier: vuông = điểm nối (nằm trên cong), tròn = điểm điều khiển; từ đoạn thứ 2 chỉ cần click 2 điểm, điểm thứ nhất tự đặt đối xứng để giữ C1 |
| Chuột phải | Xóa điểm (Bezier xóa cả đoạn chứa điểm đó) |
| S | Bezier nối C1 ↔ B-spline bậc 3 đều |
| T | Chạy t từ 0 đến 1, vẽ các đoạn nội suy de Casteljau |
| N | Hiện pháp tuyến trên mặt tròn xoay |
| P | Nạp lần lượt 5 mẫu có sẵn |
| W / R | Lưu / tải điểm điều khiển (`profile.txt`) |
| X | Xuất lưới ra `vat_tron_xoay.obj` (mở bằng Blender, hoặc `./build/phong_trung_bay vat_tron_xoay.obj`) |
| Enter | Đặt vật vừa soạn lên bục của vật đang chọn (hoặc bục 1) |
| Backspace | Xóa hết điểm |
| E | Về phòng trưng bày |

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
src/scene/      phần B (camera, phòng, xén, chọn vật, nạp .obj, nạp texture, bảo tàng, nhân vật)
src/third_party/ stb_image.h (nạp ảnh, public domain)
src/render/     phần C (renderScene đã nối sẵn các bước)
src/platform/   chọn header GLUT theo hệ điều hành
assets/         mô hình .obj mẫu (ấm trà Utah), ảnh texture, tranh (paintings/)
tools/          gen_textures.py: tạo ảnh texture
tests/          make check
```
