#pragma once
// PHAN B — CANH VA CAMERA (Hao). Slide CG.4, Lab 04.
#include <vector>

#include "mesh.hpp"
#include "pipeline.hpp"

struct Camera {
    enum class Mode { Walk, Orbit } mode = Mode::Walk;

    // Che do di bo
    Vec3 eye{0, 1.6f, 4};
    float yaw = 0, pitch = 0;    // radian; pitch gioi han +-89 do

    // Che do orbit
    Vec3 target{0, 1, 0};
    float orbitRadius = 3, orbitTheta = 0, orbitPhi = 0.3f;

    Projection projection = Projection::Perspective;
    float fovyDeg = 55, nearPlane = 0.1f, farPlane = 100;

    Mat4 viewMatrix() const;                       // ma tran look-at tu cai, khong dung gluLookAt
    Mat4 projectionMatrix(float aspect) const;     // phoi canh hoac song song
    Vec3 eyePosition() const;                      // vi tri mat trong the gioi (cho anh sang)
};

// Dung phong trung bay: san, tuong, buc va cac vat.
std::vector<SceneObject> buildRoom();

// Xen tam giac (he quan sat) theo mat phang gan z = -nearPlane (Sutherland–Hodgman).
// Tra ve da giac 0, 3 hoac 4 dinh; renderer chia lai thanh tam giac.
std::vector<ViewVertex> clipNear(const ViewVertex tri[3], float nearPlane);
