#pragma once

#include <cmath>
#include <cstddef>

struct Vector2 { float x = 0.0f; float y = 0.0f; };

struct Vector3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3() = default;
    Vector3(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue) {}
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator-() const { return {-x, -y, -z}; }
    Vector3 operator*(float value) const { return {x * value, y * value, z * value}; }
    Vector3 operator/(float value) const { return {x / value, y / value, z / value}; }
    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
};
inline Vector3 operator*(float value, const Vector3& v) { return v * value; }
inline float Dot(const Vector3& a, const Vector3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vector3 Cross(const Vector3& a, const Vector3& b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
inline float LengthSquared(const Vector3& v) { return Dot(v, v); }
inline float Length(const Vector3& v) { return std::sqrt(LengthSquared(v)); }
inline Vector3 Normalize(const Vector3& v) { const float n = Length(v); return n > 1e-6f ? v / n : Vector3{}; }
inline float Clamp(float v, float low, float high) { return v < low ? low : v > high ? high : v; }

struct Matrix4
{
    float value[16]{};
    static Matrix4 Identity();
    static Matrix4 Translation(const Vector3& v);
    static Matrix4 Scale(const Vector3& v);
    static Matrix4 Rotation(const struct Quaternion& q);
    Vector3 TransformPoint(const Vector3& p) const;
    Vector3 TransformVector(const Vector3& v) const;
    Matrix4 operator*(const Matrix4& rhs) const;
    Matrix4 Inverse() const;
};

struct Quaternion
{
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;
    Quaternion() = default;
    Quaternion(float wValue, float xValue, float yValue, float zValue) : w(wValue), x(xValue), y(yValue), z(zValue) {}
    static Quaternion FromAxisAngle(const Vector3& axis, float radians);
    Quaternion operator*(const Quaternion& q) const;
    Quaternion Normalized() const;
    Vector3 Rotate(const Vector3& v) const;
};

inline float Radians(float degrees) { return degrees * 0.01745329251994329577f; }
inline float Degrees(float radians) { return radians * 57.29577951308232f; }
