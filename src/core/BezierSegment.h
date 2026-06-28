#pragma once
#include "Vec2.h"
#include <vector>
#include <utility>

// Cubic Bezier segment defined by four control points.
//
// Formula:  B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3,  t ∈ [0,1]
// Tangent:  B'(t) = 3(1-t)²(P1-P0) + 6(1-t)t(P2-P1) + 3t²(P3-P2)
//
// G1 continuity between adjacent segments is enforced externally by keeping
// p2_prev, p3_prev==p0_next, p1_next collinear.
class BezierSegment {
public:
    Vec2 p0, p1, p2, p3; // p0/p3 = endpoints, p1/p2 = handles

    BezierSegment() = default;
    BezierSegment(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3);

    // Position at parameter t
    Vec2 evaluate(double t) const;

    // Tangent vector at parameter t (not normalised)
    Vec2 tangent(double t) const;

    // Heading in degrees: 0=north (+Y), clockwise positive (VEX convention)
    double headingAt(double t) const;

    // Approximate arc length via numerical sampling
    double arcLength(int samples = 200) const;

    // Lookup table: each entry is {t, cumulative_arc_length_from_t0}
    std::vector<std::pair<double, double>> buildArcLengthTable(int samples = 200) const;
};
