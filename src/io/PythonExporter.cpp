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

PythonOutput PythonExporter::generate(const Path& path, double stepCm,
                                      Format fmt, const RobotConfig& cfg) {
    if (path.isEmpty())
        return {"# Path has no segments.", "# Path has no segments.", 0};

    auto wps = PathInterpolator::interpolate(path, stepCm);
    PathInterpolator::computeVelocities(wps, cfg);

    QString var = toVarName(path.name);
    int     n   = static_cast<int>(wps.size());

    QString wp, hd;

    if (fmt == Format::Inline) {
        // ── Inline: var = [(x, y, v), ...] on a single line ─────────────────
        wp = var + " = [";
        for (int i = 0; i < n; ++i) {
            if (i) wp += ", ";
            wp += QString("(%1, %2, %3)")
                      .arg(wps[i].x,        0, 'f', 2)
                      .arg(wps[i].y,        0, 'f', 2)
                      .arg(wps[i].velocity, 0, 'f', 2);
        }
        wp += "]";

        hd = var + "_headings = [";
        for (int i = 0; i < n; ++i) {
            if (i) hd += ", ";
            hd += QString("%1").arg(wps[i].heading, 0, 'f', 2);
        }
        hd += "]";
    } else {
        // ── Multiline: one entry per line ────────────────────────────────────
        wp = var + " = [\n";
        for (const auto& w : wps)
            wp += QString("    (%1, %2, %3),\n")
                      .arg(w.x,        0, 'f', 2)
                      .arg(w.y,        0, 'f', 2)
                      .arg(w.velocity, 0, 'f', 2);
        wp += "]";

        hd = var + "_headings = [\n";
        for (const auto& w : wps)
            hd += QString("    %1,\n").arg(w.heading, 0, 'f', 2);
        hd += "]";
    }

    return {wp, hd, n};
}
