#include "ProjectSerializer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

// ── helpers ───────────────────────────────────────────────────────────────

static QJsonArray encodeVec2(Vec2 v) {
    return QJsonArray{v.x, v.y};
}

static Vec2 decodeVec2(const QJsonArray& a) {
    return {a[0].toDouble(), a[1].toDouble()};
}

static QJsonArray encodeSegment(const BezierSegment& s) {
    return QJsonArray{
        encodeVec2(s.p0), encodeVec2(s.p1),
        encodeVec2(s.p2), encodeVec2(s.p3)
    };
}

static BezierSegment decodeSegment(const QJsonArray& a) {
    return {decodeVec2(a[0].toArray()), decodeVec2(a[1].toArray()),
            decodeVec2(a[2].toArray()), decodeVec2(a[3].toArray())};
}

// ── public ────────────────────────────────────────────────────────────────

QByteArray ProjectSerializer::toJson(const Project& p) {
    QJsonObject root;
    root["v"]    = 1;
    root["name"] = QString::fromStdString(p.name);
    root["step"] = p.stepSizeCm;

    QJsonObject robot;
    robot["w"]        = p.robotConfig.widthCm;
    robot["h"]        = p.robotConfig.heightCm;
    robot["x"]        = p.robotConfig.startX;
    robot["y"]        = p.robotConfig.startY;
    robot["heading"]  = p.robotConfig.startHeading;
    robot["kCurve"]   = p.robotConfig.kCurve;
    robot["vMin"]     = p.robotConfig.vMin;
    robot["aMax"]     = p.robotConfig.aMax;
    robot["lookAhead"]= p.robotConfig.lookAheadCm;
    robot["vEnd"]     = p.robotConfig.vEnd;
    root["robot"]     = robot;

    QJsonArray paths;
    for (const auto& path : p.paths) {
        if (path.segments.empty()) continue;   // skip transient empty paths

        QJsonObject obj;
        obj["name"]    = QString::fromStdString(path.name);
        obj["color"]   = QJsonArray{path.color.r, path.color.g,
                                    path.color.b, path.color.a};
        obj["visible"] = path.visible;

        QJsonArray segs;
        for (const auto& seg : path.segments)
            segs.append(encodeSegment(seg));
        obj["segments"] = segs;
        paths.append(obj);
    }
    root["paths"] = paths;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool ProjectSerializer::fromJson(const QByteArray& data, Project& out) {
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    auto root      = doc.object();
    out.name       = root["name"].toString("Untitled Project").toStdString();
    out.stepSizeCm = root["step"].toDouble(2.0);

    auto robot                   = root["robot"].toObject();
    out.robotConfig.widthCm      = robot["w"].toDouble(38.0);
    out.robotConfig.heightCm     = robot["h"].toDouble(38.0);
    out.robotConfig.startX       = robot["x"].toDouble(0.0);
    out.robotConfig.startY       = robot["y"].toDouble(0.0);
    out.robotConfig.startHeading = robot["heading"].toDouble(0.0);
    out.robotConfig.kCurve       = robot["kCurve"].toDouble(25.0);
    out.robotConfig.vMin         = robot["vMin"].toDouble(0.15);
    out.robotConfig.aMax         = robot["aMax"].toDouble(0.005);
    out.robotConfig.lookAheadCm  = robot["lookAhead"].toDouble(20.0);
    out.robotConfig.vEnd         = robot["vEnd"].toDouble(0.0);

    out.paths.clear();
    for (const auto& pv : root["paths"].toArray()) {
        auto obj = pv.toObject();
        Path path(obj["name"].toString("Path").toStdString());

        auto ca = obj["color"].toArray();
        if (ca.size() == 4) {
            path.color.r = static_cast<uint8_t>(ca[0].toInt(66));
            path.color.g = static_cast<uint8_t>(ca[1].toInt(133));
            path.color.b = static_cast<uint8_t>(ca[2].toInt(244));
            path.color.a = static_cast<uint8_t>(ca[3].toInt(255));
        }
        path.visible = obj["visible"].toBool(true);

        for (const auto& sv : obj["segments"].toArray())
            path.segments.push_back(decodeSegment(sv.toArray()));

        if (!path.segments.empty())
            out.paths.push_back(std::move(path));
    }

    return true;
}

bool ProjectSerializer::saveToFile(const Project& p, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(toJson(p));
    return true;
}

bool ProjectSerializer::loadFromFile(const QString& path, Project& out) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    return fromJson(f.readAll(), out);
}
