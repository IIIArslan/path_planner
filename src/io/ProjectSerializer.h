#pragma once
#include <QByteArray>
#include <QString>
#include "core/Project.h"

class ProjectSerializer {
public:
    // Serialize full project to compact JSON bytes.
    // Empty paths (transient drawing state) are excluded.
    static QByteArray toJson(const Project& p);

    // Deserialize from JSON bytes. Returns false on parse error.
    static bool fromJson(const QByteArray& data, Project& out);

    static bool saveToFile(const Project& p, const QString& path);
    static bool loadFromFile(const QString& path, Project& out);
};
