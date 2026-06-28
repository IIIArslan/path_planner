#pragma once
#include "Path.h"
#include "RobotConfig.h"
#include <vector>
#include <string>

enum class FieldType {
    HighStakes_2025,
    OverUnder_2024,
    Empty,
    Custom
};

struct FieldConfig {
    FieldType   type            = FieldType::HighStakes_2025;
    std::string customImagePath;
};

// Top-level data model. Owns all paths and configuration.
// Modified flag is set on every mutating operation and cleared on save.
class Project {
public:
    std::string name       = "Untitled Project";
    FieldConfig fieldConfig;
    RobotConfig robotConfig;
    double      stepSizeCm = 2.0;
    std::vector<Path> paths;

    Project() = default;

    Path& addPath(const std::string& name = "");
    void  removePath(int index);
    void  mergePaths(int indexA, int indexB); // merges B into A, removes B

    bool isModified() const { return m_modified; }
    void markModified()     { m_modified = true;  }
    void markSaved()        { m_modified = false; }

private:
    bool m_modified    = false;
    int  m_pathCounter = 0;
};
