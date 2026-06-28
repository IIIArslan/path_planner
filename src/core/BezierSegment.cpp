#include "BezierSegment.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BezierSegment::BezierSegment(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3)
    : p0(p0), p1(p1), p2(p2), p3(p3) {}

Vec2 BezierSegment::evaluate(double t) const {
    double mt  = 1.0 - t;
    double mt2 = mt * mt;
    double mt3 = mt2 * mt;
    double t2  = t * t;
    double t3  = t2 * t;
    return p0 * mt3
         + p1 * (3.0 * mt2 * t)
         + p2 * (3.0 * mt  * t2)
         + p3 * t3;
}

Vec2 BezierSegment::tangent(double t) const {
    double mt = 1.0 - t;
    return (p1 - p0) * (3.0 * mt * mt)
         + (p2 - p1) * (6.0 * mt * t)
         + (p3 - p2) * (3.0 * t  * t);
}

double BezierSegment::headingAt(double t) const {
    Vec2 tan = tangent(t);

    // Degenerate tangent: sample a nearby t
    if (tan.length() < 1e-10) {
        tan = (t < 1.0) ? tangent(t + 1e-4) : tangent(t - 1e-4);
    }

    // Standard math angle: 0° = +X (east), CCW positive
    double mathDeg = std::atan2(tan.y, tan.x) * 180.0 / M_PI;

    // VEX heading: 0° = +Y (north), CW positive
    double vex = 90.0 - mathDeg;
    while (vex <   0.0) vex += 360.0;
    while (vex >= 360.0) vex -= 360.0;
    return vex;
}

double BezierSegment::arcLength(int samples) const {
    double len  = 0.0;
    Vec2   prev = evaluate(0.0);
    for (int i = 1; i <= samples; ++i) {
        Vec2 curr = evaluate(static_cast<double>(i) / samples);
        len += prev.distanceTo(curr);
        prev = curr;
    }
    return len;
}

std::vector<std::pair<double, double>> BezierSegment::buildArcLengthTable(int samples) const {
    std::vector<std::pair<double, double>> table;
    table.reserve(samples + 1);

    double cum  = 0.0;
    Vec2   prev = evaluate(0.0);
    table.push_back({0.0, 0.0});

    for (int i = 1; i <= samples; ++i) {
        double t    = static_cast<double>(i) / samples;
        Vec2   curr = evaluate(t);
        cum += prev.distanceTo(curr);
        table.push_back({t, cum});
        prev = curr;
    }
    return table;
}
