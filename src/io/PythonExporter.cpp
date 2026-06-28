#include "PythonExporter.h"
#include "core/PathInterpolator.h"
#include <QRegularExpression>

QString PythonExporter::toVarName(const std::string& name) {
    QString s = QString::fromStdString(name);
    s.replace(' ', '_');
    s.replace(QRegularExpression("[^a-zA-Z0-9_]"), "");
    if (s.isEmpty() || s[0].isDigit()) s.prepend("path_");
    return s;
}

PythonOutput PythonExporter::generate(const Path& path, double stepCm) {
    if (path.isEmpty())
        return {"# Path has no segments.", "# Path has no segments.", 0};

    auto       wps     = PathInterpolator::interpolate(path, stepCm);
    QString    var     = toVarName(path.name);
    int        n       = static_cast<int>(wps.size());

    // ── Waypoints ────────────────────────────────────────────────────────────
    QString wp = var + " = [\n";
    for (const auto& w : wps)
        wp += QString("    (%1, %2),\n").arg(w.x, 0, 'f', 2).arg(w.y, 0, 'f', 2);
    wp += "]";

    // ── Headings ─────────────────────────────────────────────────────────────
    QString hd = var + "_headings = [\n";
    for (const auto& w : wps)
        hd += QString("    %1,\n").arg(w.heading, 0, 'f', 2);
    hd += "]";

    return {wp, hd, n};
}
