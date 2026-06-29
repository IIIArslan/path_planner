#pragma once
#include "Path.h"
#include "Waypoint.h"
#include "RobotConfig.h"
#include <vector>

// Converts a piecewise Bezier Path into uniformly-spaced Waypoints and
// optionally computes a velocity profile for each point.
class PathInterpolator {
public:
    static std::vector<Waypoint> interpolate(const Path& path, double stepCm = 2.0);

    // Fills Waypoint::velocity using lookahead curvature + trapezoidal ramp.
    // Call after interpolate(). Modifies wps in-place.
    static void computeVelocities(std::vector<Waypoint>& wps, const RobotConfig& cfg);

private:
    struct TableEntry {
        double globalArc;
        int    segIdx;
        double t;
    };

    static std::vector<TableEntry> buildGlobalTable(const Path& path, int samplesPerSeg = 200);
};
