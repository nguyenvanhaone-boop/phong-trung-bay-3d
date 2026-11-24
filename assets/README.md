# Mô hình mẫu

| File | Nguồn | Ghi chú |
| --- | --- | --- |
| `teapot.obj` | Ấm trà Utah (Martin Newell, 1975), bản lưới tam giác lấy từ [alecjacobson/common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models) | 3644 đỉnh, 6320 tam giác, chỉ có `v` và `f` (pháp tuyến do bộ đọc tự tính) |

Muốn thử vật khác: `./build/phong_trung_bay duong/dan/vat.obj`. Vật được thu về cao 0,45 m và đặt lên bục giữa phòng.

## Texture

| File | Nguồn | Dùng cho |
| --- | --- | --- |
| `textures/san_go.png` | Nhóm tự tạo bằng `python3 tools/gen_textures.py` | Sàn: 5 tấm ván gỗ, 256 × 256 ứng với 1 m × 1 m |
| `textures/da_cam_thach.png` | Nhóm tự tạo bằng `python3 tools/gen_textures.py` | Các bục, sàn phòng cổ vật: đá cẩm thạch xám |
| `textures/dong_son.png` | Nhóm tự tạo bằng `python3 tools/gen_textures.py` | Trống đồng, thạp đồng: hoa văn kiểu Đông Sơn (20 % trên cùng của ảnh phủ mặt trống, ngôi sao 14 cánh ở tâm) |
| `textures/hoa_van_tuong.png` | Nhóm tự tạo bằng `python3 tools/gen_textures.py` | Giấy dán tường: lưới trám, bông hoa 8 cánh, ảnh xám nhạt nhân với màu tường từng khu, lặp mỗi 2 m |
| `textures/gom_hoa_lam.png` | Nhóm tự tạo bằng `python3 tools/gen_textures.py` | Bình gốm hoa lam: men trắng, vẽ xanh coban, lặp 8 lần theo chiều dọc (mỗi đoạn profile một bộ hoa văn) |

## Tranh (`paintings/`)

12 bức tranh treo ở hai hành lang đều là **public domain** theo Wikimedia Commons (mục "Licensing" của từng file). Ảnh được tải từ bản thu nhỏ của Commons, rồi thu tiếp về cạnh dài 512 px, JPEG chất lượng 82, để repo nhẹ (tổng khoảng 1,1 MB).

| File | Tác phẩm | Nguồn (Wikimedia Commons) |
| --- | --- | --- |
| `dam_cuoi_chuot.jpg` | Đám cưới chuột, tranh Đông Hồ | [Đám cưới chuột.JPG](https://commons.wikimedia.org/wiki/File:%C4%90%C3%A1m_c%C6%B0%E1%BB%9Bi_chu%E1%BB%99t.JPG) |
| `phu_dong_thien_vuong.jpg` | Phù Đổng Thiên Vương, tranh Đông Hồ (thế kỷ 19) | [Tranh Đông Hồ vẽ Phù Đổng Thiên Vương.jpg](https://commons.wikimedia.org/wiki/File:Tranh_%C4%90%C3%B4ng_H%E1%BB%93_v%E1%BA%BD_Ph%C3%B9_%C4%90%E1%BB%95ng_Thi%C3%AAn_V%C6%B0%C6%A1ng.jpg) |
| `hai_ba_trung.jpg` | Hai Bà Trưng, tranh Đông Hồ | [Hai ba trung Dong Ho painting.jpg](https://commons.wikimedia.org/wiki/File:Hai_ba_trung_Dong_Ho_painting.jpg) |
| `lon_am_duong.jpg` | Lợn âm dương, tranh Đông Hồ | [Lợn âm dương.JPG](https://commons.wikimedia.org/wiki/File:L%E1%BB%A3n_%C3%A2m_d%C6%B0%C6%A1ng.JPG) |
| `muc_dong_doc_sach.jpg` | Mục đồng đọc sách, tranh Đông Hồ | [Dong Ho painting - Muc dong doc sach.jpg](https://commons.wikimedia.org/wiki/File:Dong_Ho_painting_-_Muc_dong_doc_sach.jpg) |
| `song_lon_kanagawa.jpg` | Sóng lớn ngoài khơi Kanagawa, Hokusai (khoảng 1831) | [Tsunami by hokusai 19th century.jpg](https://commons.wikimedia.org/wiki/File:Tsunami_by_hokusai_19th_century.jpg) |
| `mona_lisa.jpg` | Mona Lisa, Leonardo da Vinci (1503–1506) | [Mona Lisa, by Leonardo da Vinci, from C2RMF retouched.jpg](https://commons.wikimedia.org/wiki/File:Mona_Lisa,_by_Leonardo_da_Vinci,_from_C2RMF_retouched.jpg) |
| `thieu_nu_ngoc_trai.jpg` | Thiếu nữ đeo hoa tai ngọc trai, Vermeer (khoảng 1665) | [1665 Girl with a Pearl Earring.jpg](https://commons.wikimedia.org/wiki/File:1665_Girl_with_a_Pearl_Earring.jpg) |
| `dem_day_sao.jpg` | Đêm đầy sao, Van Gogh (1889) | [Van Gogh - Starry Night - Google Art Project.jpg](https://commons.wikimedia.org/wiki/File:Van_Gogh_-_Starry_Night_-_Google_Art_Project.jpg) |
| `an_tuong_mat_troi_moc.jpg` | Ấn tượng, mặt trời mọc, Monet (1872) | [Monet - Impression, Sunrise.jpg](https://commons.wikimedia.org/wiki/File:Monet_-_Impression,_Sunrise.jpg) |
| `nu_hon.jpg` | Nụ hôn, Klimt (1907–1908) | [The Kiss - Gustav Klimt - Google Cultural Institute.jpg](https://commons.wikimedia.org/wiki/File:The_Kiss_-_Gustav_Klimt_-_Google_Cultural_Institute.jpg) |
| `tho_san_tren_tuyet.jpg` | Thợ săn trên tuyết, Bruegel cha (1565) | [Pieter Bruegel the Elder - Hunters in the Snow (Winter).jpg](https://commons.wikimedia.org/wiki/File:Pieter_Bruegel_the_Elder_-_Hunters_in_the_Snow_(Winter)_-_Google_Art_Project.jpg) |

Thiếu file ảnh nào thì bức đó hiện bàn cờ xám (chương trình vẫn chạy, in cảnh báo ra stderr).

Cổ vật trong phòng cổ vật là **mô hình mô phỏng** do nhóm dựng bằng mặt tròn xoay, không phải quét 3D hiện vật thật.

Ảnh được nạp bằng [stb_image](https://github.com/nothings/stb) v2.32 (public domain, nằm ở `src/third_party/stb_image.h`). Ảnh lặp liền mạch, vì hình hộp đặt (u, v) theo mét nên mỗi 1 m ảnh lặp lại một lần.
