#pragma once
#include "Path.h"
#include "Waypoint.h"
#include <vector>

// Converts a piecewise Bezier Path into a list of Waypoints with
// approximately uniform arc-length spacing using a pre-built lookup table.
class PathInterpolator {
public:
    static std::vector<Waypoint> interpolate(const Path& path, double stepCm = 2.0);

private:
    struct TableEntry {
        double globalArc; // cumulative arc length from path start (cm)
        int    segIdx;
        double t;         // parameter within that segment
    };

    static std::vector<TableEntry> buildGlobalTable(const Path& path, int samplesPerSeg = 200);
};
