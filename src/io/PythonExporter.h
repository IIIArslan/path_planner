#pragma once
#include "core/Path.h"
#include <QString>

struct PythonOutput {
    QString waypointCode;
    QString headingCode;
    int     pointCount = 0;
};

// Interpolates a Path and formats the result as copy-pasteable Python lists.
class PythonExporter {
public:
    static PythonOutput generate(const Path& path, double stepCm);

private:
    // Convert path name → valid Python identifier
    static QString toVarName(const std::string& name);
};
