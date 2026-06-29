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
    enum class Format { Multiline, Inline };

    static PythonOutput generate(const Path& path, double stepCm,
                                 Format fmt = Format::Multiline);

private:
    static QString toVarName(const std::string& name);
};
