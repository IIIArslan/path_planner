#include "PathInterpolator.h"
#include <algorithm>
#include <cmath>

std::vector<PathInterpolator::TableEntry>
PathInterpolator::buildGlobalTable(const Path& path, int samplesPerSeg) {
    std::vector<TableEntry> table;
    double globalOffset = 0.0;

    for (int si = 0; si < static_cast<int>(path.segments.size()); ++si) {
        const auto& seg      = path.segments[si];
        auto        arcTable = seg.buildArcLengthTable(samplesPerSeg);

        // Skip the first point of subsequent segments (shared with previous end)
        size_t startIdx = (si == 0) ? 0 : 1;
        for (size_t i = startIdx; i < arcTable.size(); ++i) {
            table.push_back({globalOffset + arcTable[i].second, si, arcTable[i].first});
        }

        if (!arcTable.empty())
            globalOffset += arcTable.back().second;
    }
    return table;
}

std::vector<Waypoint> PathInterpolator::interpolate(const Path& path, double stepCm) {
    if (path.isEmpty() || stepCm <= 0.0) return {};

    auto table = buildGlobalTable(path);
    if (table.empty()) return {};

    double totalLen = table.back().globalArc;
    std::vector<Waypoint> waypoints;

    for (double target = 0.0; target <= totalLen + 1e-9; target += stepCm) {
        // Binary search: first entry with globalArc >= target
        auto it = std::lower_bound(table.begin(), table.end(), target,
            [](const TableEntry& e, double v) { return e.globalArc < v; });

        int    segIdx;
        double t;

        if (it == table.end()) {
            // Past the end — clamp to last point
            const auto& last = table.back();
            segIdx = last.segIdx;
            t      = last.t;
        } else if (it == table.begin()) {
            segIdx = it->segIdx;
            t      = it->t;
        } else {
            const auto& e1   = *it;
            const auto& e0   = *std::prev(it);
            double      span = e1.globalArc - e0.globalArc;
            double      frac = (span > 1e-10) ? (target - e0.globalArc) / span : 0.0;

            if (e0.segIdx == e1.segIdx) {
                segIdx = e1.segIdx;
                t      = e0.t + frac * (e1.t - e0.t);
            } else {
                // Segment boundary: choose whichever side the target is closer to
                segIdx = (frac < 0.5) ? e0.segIdx : e1.segIdx;
                t      = (frac < 0.5) ? 1.0 : 0.0;
            }
            t = std::clamp(t, 0.0, 1.0);
        }

        const auto& seg = path.segments[segIdx];
        waypoints.emplace_back(seg.evaluate(t), seg.headingAt(t));
    }

    // Always include the exact end point
    {
        const auto& lastSeg = path.segments.back();
        Vec2        endPos  = lastSeg.evaluate(1.0);
        if (waypoints.empty() || waypoints.back().position().distanceTo(endPos) > 1e-3)
            waypoints.emplace_back(endPos, lastSeg.headingAt(1.0));
    }

    return waypoints;
}
