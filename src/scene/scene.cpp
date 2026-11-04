// PHAN B — CANH VA CAMERA (Hao). Cac ham duoi day la khung: thay TODO bang thuat toan.
#include "scene.hpp"

Mat4 Camera::viewMatrix() const {
    // TODO(Hao, Sprint 2): f = normalize(C - E), s = normalize(f x up), u = s x f;
    // hang 0..2 = s, u, -f; cot tinh tien = (-s.E, -u.E, f.E). (slide CG.4, he quan sat)
    return Mat4::identity();
}

Mat4 Camera::projectionMatrix(float aspect) const {
    // TODO(Hao, Sprint 3): phoi canh (fovy, aspect, near, far) va song song (hop nhin doi xung).
    (void)aspect;
    return Mat4::identity();
}

Vec3 Camera::eyePosition() const {
    // TODO(Hao, Sprint 3): che do orbit: E = target + R (cos phi sin theta, sin phi, cos phi cos theta).
    return eye;
}

std::vector<SceneObject> buildRoom() {
    // TODO(Hao, Sprint 2): san, 4 tuong, 5 buc; nhan Mesh tu phan A va dat bang translate/scale/rotate.
    return {};
}

std::vector<ViewVertex> clipNear(const ViewVertex tri[3], float nearPlane) {
    // TODO(Hao, Sprint 3): Sutherland–Hodgman voi mot mat phang z = -near;
    // canh cat mat phang -> noi suy moi thuoc tinh (view, world, normal, u, v) theo cung tham so.
    (void)nearPlane;
    return {tri[0], tri[1], tri[2]};
}
