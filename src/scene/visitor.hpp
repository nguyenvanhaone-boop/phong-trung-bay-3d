#pragma once
// PHAN B — KHACH THAM QUAN TU DI (Hao, bao tang mo rong).
// Khach DEN tu mep san truoc, qua cua chinh (luon mo) va cong vao sanh, tham quan mot phan bao tang
// (chi sanh / sanh + hanh lang A / ca vong), roi DI RA bang duong cu va bien mat o mep san.
// main.cpp lien tuc dua khach moi vao -> luc nao cung co nguoi ra, nguoi vao.
//
// Toi diem dung: dung lai vai giay, quay mat ve buc tranh / tu kinh. Giua 2 diem: di thang,
// va cham tuong / vat can dung chung luoi WalkGrid (truot doc tuong).
// Tranh nguoi khac: co nguoi o phia truoc trong 0,75 m thi dung cho; cho lau thi buoc sang ngang.
// Ket han (khong nhuc nhich 3 giay) thi bo qua diem dung do.
#include <vector>

#include "character.hpp"
#include "museum.hpp"

struct Waypoint {
    Vec3 position;        // cho dung (tren san)
    float pause = 0;      // giay dung lai; 0 = chi di qua
    bool look = false;    // dung lai thi quay mat ve `target`
    Vec3 target;
};

struct Visitor {
    Character body;
    std::vector<Waypoint> route;
    int next = 0;            // diem dang di toi
    float wait = 0;          // con dung bao lau (giay)
    float blocked = 0;       // da cho nguoi khac bao lau
    float stuck = 0;         // bao lau khong tien duoc
    float speed = 0.55f;     // ti le so voi toc do di cua nhan vat (khach di thong tha)
    bool loop = false;       // true: di vong mai; false: het lo trinh thi ra ve
    bool done = false;       // da ra toi mep san -> main xoa khach nay

    // others: vi tri nhung nguoi khac (nhan vat dieu khien, khach khac) de tranh.
    void update(float dt, const WalkGrid& grid, const std::vector<Vec3>& others);
};

// Vong tham quan ben trong (sanh -> hanh lang A -> phong co vat -> hanh lang B -> sanh).
std::vector<Waypoint> museumTour();

// Lo trinh 1 luot khach: vao tu san truoc, tham quan theo `plan` (0 = chi sanh, 1 = sanh + hanh lang A
// roi quay lai, 2 = ca vong), ra cua chinh, ve mep san. `lane` (-1..1) lech duong vao / ra cho do trung nhau.
std::vector<Waypoint> visitRoute(int plan, float lane);

// Khach moi den o mep san truoc. `seed` chon ke hoach, quan ao, toc do.
Visitor makeArrival(int seed);

// `count` khach da o san trong bao tang luc mo chuong trinh (rai deu tren cac lo trinh).
std::vector<Visitor> makeVisitors(int count);
