#pragma once

struct RobotConfig {
    double widthCm      = 38.0;
    double heightCm     = 38.0;
    double startX       = 0.0;
    double startY       = 0.0;
    double startHeading = 0.0;  // degrees: 0=north, clockwise positive

    // Velocity profile parameters
    double kCurve      = 25.0;  // curvature sensitivity (cm/rad): higher → slower in curves
    double vMin        = 0.15;  // minimum velocity [0–1]
    double aMax        = 0.025; // max accel/decel (Δv²/cm): v² = v₀² + 2·aMax·Δd
    double lookAheadCm = 20.0;  // lookahead window for predictive curvature check (cm)
    double vEnd        = 0.0;   // target velocity at path end [0–1]
};
