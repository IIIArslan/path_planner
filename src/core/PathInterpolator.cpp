#include "PathInterpolator.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

void PathInterpolator::computeVelocities(std::vector<Waypoint>& wps,
                                          const RobotConfig& cfg)
{
    int n = static_cast<int>(wps.size());
    if (n == 0) return;
    if (n == 1) { wps[0].velocity = std::clamp(cfg.vEnd, 0.0, 1.0); return; }

    // ── Pairwise distances and curvatures ──────────────────────────────────
    std::vector<double> dists(n - 1);
    std::vector<double> kappa(n, 0.0);

    for (int i = 0; i + 1 < n; ++i) {
        double dx = wps[i+1].x - wps[i].x;
        double dy = wps[i+1].y - wps[i].y;
        dists[i]  = std::max(1e-9, std::sqrt(dx*dx + dy*dy));

        double dh = wps[i+1].heading - wps[i].heading;
        while (dh >  180.0) dh -= 360.0;
        while (dh < -180.0) dh += 360.0;
        kappa[i] = std::abs(dh) * (M_PI / 180.0) / dists[i];
    }
    kappa[n - 1] = kappa[n - 2];

    // ── Lookahead-window velocity ceiling ──────────────────────────────────
    // For each point i, look up to lookAheadCm ahead; use worst curvature found.
    std::vector<double> vCeil(n);
    for (int i = 0; i < n; ++i) {
        double kMax    = kappa[i];
        double cumDist = 0.0;
        for (int j = i + 1; j < n; ++j) {
            cumDist += dists[j - 1];
            if (cumDist > cfg.lookAheadCm) break;
            kMax = std::max(kMax, kappa[j]);
        }
        double v  = 1.0 / (1.0 + cfg.kCurve * kMax);
        vCeil[i]  = std::clamp(v, cfg.vMin, 1.0);
    }

    // ── Forward pass: acceleration ramp from rest ───────────────────────────
    std::vector<double> vel(n);
    vel[0] = 0.0;
    for (int i = 1; i < n; ++i) {
        double vAccel = std::sqrt(vel[i-1] * vel[i-1] + 2.0 * cfg.aMax * dists[i-1]);
        vel[i] = std::min(vCeil[i], vAccel);
    }

    // ── Backward pass: deceleration ramp to vEnd ───────────────────────────
    vel[n - 1] = std::min(vel[n - 1], std::clamp(cfg.vEnd, 0.0, 1.0));
    for (int i = n - 2; i >= 0; --i) {
        double vDecel = std::sqrt(vel[i+1] * vel[i+1] + 2.0 * cfg.aMax * dists[i]);
        vel[i] = std::min(vel[i], vDecel);
    }

    for (int i = 0; i < n; ++i)
        wps[i].velocity = std::clamp(vel[i], 0.0, 1.0);
}
