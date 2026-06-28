#pragma once

struct RobotConfig {
    double widthCm      = 38.0; // physical robot width
    double heightCm     = 38.0; // physical robot depth (front-to-back)
    double startX       = 0.0;  // initial position (cm, field-centre origin)
    double startY       = 0.0;
    double startHeading = 0.0;  // degrees: 0=north, clockwise positive
};
