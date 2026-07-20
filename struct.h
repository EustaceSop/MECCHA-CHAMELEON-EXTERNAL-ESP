#pragma once
#include <cmath>

struct vec2 {
    float x, y;
};

// UE5 uses double for FVector, but camera POV still uses float in FMinimalViewInfo
struct Vector3 {
    float x, y, z;

    Vector3 operator-(Vector3 v) { return { x - v.x, y - v.y, z - v.z }; }
    Vector3 operator+(Vector3 v) { return { x + v.x, y + v.y, z + v.z }; }
    Vector3 operator*(float s) { return { x * s, y * s, z * s }; }
    Vector3 operator/(float s) { return { x / s, y / s, z / s }; }

    float Length() { return sqrtf(x * x + y * y + z * z); }
    float Length2D() { return sqrtf(x * x + y * y); }
    float DistTo(Vector3 v) { return (*this - v).Length(); }
    float Dot(Vector3& v) { return x * v.x + y * v.y + z * v.z; }
};

// UE5 FVector uses doubles
struct FVector {
    double x, y, z;
};

struct FRotator {
    double pitch, yaw, roll;
};

float ToMeters(float x) {
    return x / 100.f;
}
