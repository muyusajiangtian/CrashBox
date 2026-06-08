#pragma once
// 数学工具函数和向量操作

#include <cmath>
#include <algorithm>

namespace CrashBox {

// 二维向量
struct Vec2 {
    float x = 0.0f, y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }

    Vec2 normalized() const {
        float len = length();
        if (len < 1e-6f) return {0, 0};
        return {x / len, y / len};
    }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float cross(const Vec2& o) const { return x * o.y - y * o.x; }

    Vec2 perpCW() const { return {y, -x}; }
    Vec2 perpCCW() const { return {-y, x}; }
};

inline Vec2 operator*(float s, const Vec2& v) { return {s * v.x, s * v.y}; }

// 限制值在范围内
inline float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

// 线性插值
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// 角度转弧度
inline float degToRad(float deg) { return deg * 3.14159265f / 180.0f; }
inline float radToDeg(float rad) { return rad * 180.0f / 3.14159265f; }

constexpr float PI = 3.14159265358979f;
constexpr float GRAVITY = 9.81f;

} // namespace CrashBox
