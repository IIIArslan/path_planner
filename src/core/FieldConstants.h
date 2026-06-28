#pragma once

namespace Field {
    // VEX V5 competition field: 144 × 144 inches = 365.76 × 365.76 cm
    static constexpr double SIZE_CM = 365.76;
    static constexpr double HALF_CM = SIZE_CM / 2.0; // 182.88
    static constexpr double TILE_CM = SIZE_CM / 6.0;  // 60.96
    static constexpr int    TILES   = 6;
}
