#pragma once
#include <cmath>

struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    Vec2() = default;
    Vec2(double x, double y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(double s)      const { return {x * s,   y * s};   }
    Vec2 operator/(double s)      const { return {x / s,   y / s};   }

    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }

    double length() const { return std::sqrt(x * x + y * y); }
    double dot(const Vec2& o) const { return x * o.x + y * o.y; }

    Vec2 normalize() const {
        double l = length();
        return l > 1e-10 ? *this / l : Vec2{};
    }

    double distanceTo(const Vec2& o) const { return (*this - o).length(); }

    static Vec2 lerp(const Vec2& a, const Vec2& b, double t) {
        return a + (b - a) * t;
    }
};
