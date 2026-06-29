#pragma once
#include "Path.h"
#include "Waypoint.h"
#include <vector>

// Converts a piecewise Bezier Path into uniformly-spaced Waypoints.
class PathInterpolator {
public:
    static std::vector<Waypoint> interpolate(const Path& path, double stepCm = 2.0);

private:
    struct TableEntry {
        double globalArc;
        int    segIdx;
        double t;
    };

    static std::vector<TableEntry> buildGlobalTable(const Path& path, int samplesPerSeg = 200);
};
