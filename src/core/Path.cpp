#include "Path.h"

Vec2 Path::startPoint() const {
    return isEmpty() ? Vec2{} : segments.front().p0;
}

Vec2 Path::endPoint() const {
    return isEmpty() ? Vec2{} : segments.back().p3;
}

double Path::totalLength() const {
    double total = 0.0;
    for (const auto& seg : segments)
        total += seg.arcLength();
    return total;
}

void Path::merge(const Path& other) {
    for (const auto& seg : other.segments)
        segments.push_back(seg);
}
