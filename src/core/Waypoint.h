#pragma once
#include "Vec2.h"

// A single interpolated point on a path.
// Coordinate unit: cm. Origin: field centre.
// Heading: 0° = north (+Y), clockwise positive (VEX V5 standard).
// Velocity: fraction of max speed [0.0–1.0], filled by computeVelocities().
struct Waypoint {
    double x        = 0.0;
    double y        = 0.0;
    double heading  = 0.0;
    double velocity = 1.0;

    Waypoint() = default;
    Waypoint(double x, double y, double heading) : x(x), y(y), heading(heading) {}
    Waypoint(const Vec2& pos, double heading) : x(pos.x), y(pos.y), heading(heading) {}

    Vec2 position() const { return {x, y}; }
};
