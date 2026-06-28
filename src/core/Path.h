#pragma once
#include "BezierSegment.h"
#include <vector>
#include <string>
#include <cstdint>

struct PathColor {
    uint8_t r = 66, g = 133, b = 244, a = 255; // default: blue
};

// A named, styled collection of piecewise cubic Bezier segments.
// Adjacent segments share their endpoint (segments[i].p3 == segments[i+1].p0).
// G1 continuity (smooth join) is maintained by the editor when the user moves handles.
class Path {
public:
    std::string name    = "Path";
    PathColor   color;
    bool        visible = true;
    std::vector<BezierSegment> segments;

    Path() = default;
    explicit Path(std::string name) : name(std::move(name)) {}

    bool   isEmpty()     const { return segments.empty(); }
    Vec2   startPoint()  const;
    Vec2   endPoint()    const;
    double totalLength() const;

    // Append all segments from other (caller ensures endpoint continuity)
    void merge(const Path& other);
};
