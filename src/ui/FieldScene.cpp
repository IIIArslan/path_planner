#include "FieldScene.h"
#include "core/FieldConstants.h"
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPixmap>
#include <QGraphicsSceneMouseEvent>
#include <algorithm>

// ── construction ───────────────────────────────────────────────────────────

FieldScene::FieldScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-Field::HALF_CM, -Field::HALF_CM, Field::SIZE_CM, Field::SIZE_CM);
    buildBackground();
    buildGrid();

    // Robot item (always present; repositioned when project is loaded)
    m_robotItem = new RobotItem;
    addItem(m_robotItem);
    connect(m_robotItem, &RobotItem::poseChanged, this, [this](double x, double y, double h) {
        if (m_project) {
            m_project->robotConfig.startX       = x;
            m_project->robotConfig.startY       = y;
            m_project->robotConfig.startHeading = h;
        }
        emit robotMoved(x, y, h);
    });

    // Preview line (dashed, cosmetic)
    QPen previewPen(QColor(200, 200, 200, 100), 1.0, Qt::DashLine);
    previewPen.setCosmetic(true);
    m_previewLine = addLine(QLineF(), previewPen);
    m_previewLine->setZValue(15);
    m_previewLine->setVisible(false);
}

// ── public API ─────────────────────────────────────────────────────────────

void FieldScene::setProject(Project* project) {
    // Clear existing path items
    for (auto* item : m_pathItems) delete item;
    m_pathItems.clear();

    m_project = project;
    if (!m_project) return;

    // Rebuild items for any pre-existing paths
    for (auto& path : m_project->paths)
        m_pathItems.append(createPathItem(&path));

    m_robotItem->applyConfig(m_project->robotConfig);
}

void FieldScene::loadFieldImage(const QString& path) {
    QPixmap px(path);
    if (px.isNull()) return;

    if (!m_img) {
        m_img = addPixmap(px);
        m_img->setZValue(-1);
    } else {
        m_img->setPixmap(px);
    }

    double sx = Field::SIZE_CM / px.width();
    double sy = Field::SIZE_CM / px.height();
    m_img->setPos(-Field::HALF_CM, -Field::HALF_CM);
    m_img->setTransform(QTransform::fromScale(sx, sy));

    if (m_bg) m_bg->setVisible(false);
}

void FieldScene::clearFieldImage() {
    if (m_img) { removeItem(m_img); delete m_img; m_img = nullptr; }
    if (m_bg)  m_bg->setVisible(true);
}

void FieldScene::setRobotVisible(bool v) {
    if (m_robotItem) m_robotItem->setVisible(v);
}

void FieldScene::applyRobotConfig(const RobotConfig& cfg) {
    if (m_robotItem) m_robotItem->applyConfig(cfg);
}

void FieldScene::beginEdit() {
    emit projectAboutToChange();
}

void FieldScene::startNewPath() {
    if (!m_project) return;

    // Finish any ongoing drawing first
    if (m_editMode == EditMode::DrawPath)
        finishDrawing();

    // Create new path in the model and a corresponding item
    Path& newPath    = m_project->addPath();
    auto* item       = createPathItem(&newPath);
    m_drawPathIdx    = static_cast<int>(m_pathItems.size()) - 1;
    m_hasFirstPoint  = false;
    m_editMode       = EditMode::DrawPath;

    selectPath(item);
    emit editModeChanged(m_editMode);
    emit pathCountChanged(static_cast<int>(m_project->paths.size()));
}

void FieldScene::finishDrawing() {
    if (m_editMode != EditMode::DrawPath) return;

    // If only one point was placed (no segment), remove the empty path
    if (m_drawPathIdx >= 0 && m_drawPathIdx < static_cast<int>(m_project->paths.size())) {
        if (m_project->paths[m_drawPathIdx].isEmpty()) {
            m_project->removePath(m_drawPathIdx);
            delete m_pathItems.takeAt(m_drawPathIdx);
            emit pathCountChanged(static_cast<int>(m_project->paths.size()));
        }
    }

    m_drawPathIdx   = -1;
    m_hasFirstPoint = false;
    m_previewLine->setVisible(false);
    m_editMode = EditMode::Select;
    emit editModeChanged(m_editMode);
}

void FieldScene::cancelDrawing() {
    finishDrawing(); // finishDrawing already handles the empty-path removal
}

// ── mouse events ───────────────────────────────────────────────────────────

void FieldScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (m_editMode == EditMode::DrawPath) {
        if (event->button() == Qt::LeftButton) {
            QPointF fp = sceneToField(event->scenePos());
            // Clamp to field boundary
            const double H = Field::HALF_CM;
            fp.setX(std::clamp(fp.x(), -H, H));
            fp.setY(std::clamp(fp.y(), -H, H));
            addPointToPath(fp);
            event->accept();
            return;
        }
        if (event->button() == Qt::RightButton) {
            finishDrawing();
            event->accept();
            return;
        }
    }

    // Select mode: let items handle the event, then sync selection state
    QGraphicsScene::mousePressEvent(event);

    if (m_editMode == EditMode::Select && event->button() == Qt::LeftButton) {
        // Check whether any BezierPathItem (or its children) was under the cursor
        bool hitPath = false;
        for (auto* raw : items(event->scenePos())) {
            QGraphicsItem* it = raw;
            while (it) {
                if (dynamic_cast<BezierPathItem*>(it)) { hitPath = true; break; }
                it = it->parentItem();
            }
            if (hitPath) break;
        }
        if (!hitPath)
            selectPath(nullptr); // click on empty field → deselect all
    }
}

void FieldScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_editMode == EditMode::DrawPath && m_previewLine) {
        QPointF from;
        bool showPreview = false;

        if (m_hasFirstPoint) {
            from = fieldToScene(QPointF(m_firstPoint.x, m_firstPoint.y));
            showPreview = true;
        } else if (m_drawPathIdx >= 0 &&
                   m_drawPathIdx < static_cast<int>(m_project->paths.size())) {
            const Path& path = m_project->paths[m_drawPathIdx];
            if (!path.isEmpty()) {
                Vec2 ep = path.endPoint();
                from = fieldToScene(QPointF(ep.x, ep.y));
                showPreview = true;
            }
        }

        if (showPreview) {
            m_previewLine->setLine(QLineF(from, event->scenePos()));
            m_previewLine->setVisible(true);
        } else {
            m_previewLine->setVisible(false);
        }
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void FieldScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    if (m_editMode == EditMode::DrawPath) {
        // The first click of a double-click already placed a point via mousePressEvent;
        // finish drawing on the second click without adding another point.
        finishDrawing();
        event->accept();
        return;
    }
    QGraphicsScene::mouseDoubleClickEvent(event);
}

// ── private helpers ────────────────────────────────────────────────────────

void FieldScene::addPointToPath(QPointF fieldPos) {
    if (m_drawPathIdx < 0 || !m_project) return;

    Vec2 newPt = {fieldPos.x(), fieldPos.y()};

    if (!m_hasFirstPoint) {
        // Store the first anchor; no segment yet
        m_firstPoint    = newPt;
        m_hasFirstPoint = true;
        return;
    }

    // Create a new segment from the previous anchor to newPt
    Vec2 p0, p3;
    Path& path = m_project->paths[m_drawPathIdx];

    if (path.isEmpty()) {
        p0 = m_firstPoint;
    } else {
        p0 = path.endPoint();
    }
    p3 = newPt;

    // Default handles at 1/3 and 2/3 along the chord (straight-line start)
    Vec2 p1 = p0 + (p3 - p0) * (1.0 / 3.0);
    Vec2 p2 = p0 + (p3 - p0) * (2.0 / 3.0);

    emit projectAboutToChange();   // snapshot before adding the new segment
    path.segments.emplace_back(p0, p1, p2, p3);
    m_project->markModified();

    m_pathItems[m_drawPathIdx]->refreshFromModel();

    // The "first point" is now part of the path; subsequent clicks extend it
    m_hasFirstPoint = false;
    m_firstPoint    = {};
}

void FieldScene::removePath(int idx) {
    if (!m_project || idx < 0 || idx >= static_cast<int>(m_pathItems.size())) return;
    emit projectAboutToChange();
    if (m_selectedIdx == idx)       m_selectedIdx = -1;
    else if (m_selectedIdx > idx)   --m_selectedIdx;
    m_project->removePath(idx);
    rebuildPathItems();
    emit pathCountChanged(static_cast<int>(m_project->paths.size()));
    emit pathSelectionChanged(m_selectedIdx);
}

void FieldScene::mergePathsAt(int a, int b) {
    int n = static_cast<int>(m_pathItems.size());
    if (!m_project || a < 0 || b < 0 || a >= n || b >= n || a == b) return;
    emit projectAboutToChange();
    m_project->mergePaths(a, b);   // appends b into a, erases b
    // Update selected index: if b was selected → switch to a; if b was before selected → shift down
    if      (m_selectedIdx == b) m_selectedIdx = a;
    else if (m_selectedIdx > b)  --m_selectedIdx;
    rebuildPathItems();
    emit pathCountChanged(static_cast<int>(m_project->paths.size()));
    emit pathSelectionChanged(m_selectedIdx);
}

void FieldScene::refreshPathItem(int idx) {
    if (idx >= 0 && idx < m_pathItems.size())
        m_pathItems[idx]->syncFromModel();
}

void FieldScene::selectPathAt(int idx) {
    if (idx >= 0 && idx < m_pathItems.size())
        selectPath(m_pathItems[idx]);
    else
        selectPath(nullptr);
}

BezierPathItem* FieldScene::createPathItem(Path* path) {
    auto* item = new BezierPathItem(path);
    addItem(item);
    m_pathItems.append(item);

    connect(item, &BezierPathItem::pathClicked, this, [this](BezierPathItem* clicked) {
        if (m_editMode == EditMode::Select)
            selectPath(clicked);
    });
    connect(item, &BezierPathItem::aboutToEdit, this, [this]{ emit projectAboutToChange(); });

    return item;
}

void FieldScene::selectPath(BezierPathItem* selected) {
    for (int i = 0; i < m_pathItems.size(); ++i)
        m_pathItems[i]->setEditSelected(m_pathItems[i] == selected);
    m_selectedIdx = selected ? m_pathItems.indexOf(selected) : -1;
    emit pathSelectionChanged(m_selectedIdx);
}

void FieldScene::rebuildPathItems() {
    for (auto* item : m_pathItems) { removeItem(item); delete item; }
    m_pathItems.clear();
    if (!m_project) return;
    for (auto& path : m_project->paths)
        createPathItem(&path);
    // Restore selection highlight
    if (m_selectedIdx >= 0 && m_selectedIdx < m_pathItems.size())
        m_pathItems[m_selectedIdx]->setEditSelected(true);
}

void FieldScene::buildBackground() {
    m_bg = addRect(
        -Field::HALF_CM, -Field::HALF_CM, Field::SIZE_CM, Field::SIZE_CM,
        QPen(Qt::NoPen),
        QBrush(QColor(28, 72, 42))
    );
    m_bg->setZValue(-2);
}

void FieldScene::applyFieldTemplate(int type) {
    for (auto* item : m_templateItems) { removeItem(item); delete item; }
    m_templateItems.clear();
    if (m_bg) m_bg->setVisible(true);  // restore default bg whenever template changes
    if (type == 0) return;

    const double H = Field::HALF_CM;   // 182.88 cm
    const double T = Field::TILE_CM;   // 60.96 cm

    // Semi-transparent filled rect (scene coords; top-left origin)
    auto zone = [&](double sx, double sy, double w, double h, QColor c, double alpha = 0.28) {
        c.setAlphaF(alpha);
        auto* r = addRect(sx, sy, w, h, QPen(Qt::NoPen), QBrush(c));
        r->setZValue(0.5);
        m_templateItems.append(r);
    };

    // Filled circle at field (fx, fy)
    auto dot = [&](double fx, double fy, double r, QColor fill, QColor stroke) {
        double sx = fx, sy = -fy;
        auto* e = addEllipse(sx - r, sy - r, r * 2, r * 2,
                             QPen(stroke, 1.5), QBrush(fill));
        e->setZValue(3);
        m_templateItems.append(e);
    };

    // ── VEX Override (2026-27) ───────────────────────────────────────────────
    // After 90°-CW display rotation: Blue=+X (right wall), Red=−X (left wall), Neutral=±Y
    if (type >= 3) {
        // ── Field image (embedded resource) ─────────────────────────────────
        QPixmap rawPx(":/fields/override_field.png");
        bool hasImage = !rawPx.isNull();
        if (hasImage) {
            // Rotate 90° CW so Blue=+X (right), Red=−X (left), Neutral=±Y
            QPixmap px = rawPx.transformed(QTransform().rotate(90), Qt::SmoothTransformation);
            auto* imgItem = addPixmap(px);
            double sx = Field::SIZE_CM / static_cast<double>(px.width());
            double sy = Field::SIZE_CM / static_cast<double>(px.height());
            imgItem->setPos(-H, -H);
            imgItem->setTransform(QTransform::fromScale(sx, sy));
            imgItem->setZValue(-1.5);
            m_templateItems.append(imgItem);
            if (m_bg) m_bg->setVisible(false);
        }

        // ── Drawn elements (only when no photo is available) ─────────────────
        if (!hasImage) {
            // Light mat background
            auto* mat = addRect(-H, -H, Field::SIZE_CM, Field::SIZE_CM,
                                QPen(Qt::NoPen), QBrush(QColor(215, 210, 200)));
            mat->setZValue(-1.5);
            m_templateItems.append(mat);
            // Diagonal X lines corner-to-corner (field: SW↔NE and NW↔SE)
            {
                QPen xp(QColor(160, 158, 152, 230), 14.0);
                auto* l1 = addLine(-H,  H,  H, -H, xp);
                auto* l2 = addLine(-H, -H,  H,  H, xp);
                l1->setZValue(0.2);  l2->setZValue(0.2);
                m_templateItems << l1 << l2;
            }
            // Alliance corner zones (1×1 tile, 2 per alliance)
            zone(H - T,    -H, T, T, QColor(50,  90, 210), 0.55);
            zone(H - T,  H-T,  T, T, QColor(50,  90, 210), 0.55);
            zone(-H,      -H,  T, T, QColor(200,  50,  50), 0.55);
            zone(-H,     H-T,  T, T, QColor(200,  50,  50), 0.55);
            // Match-load zones (yellow bar, 1T×2T, centred on alliance walls)
            zone(H - T,  -T, T, T * 2, QColor(240, 200, 0), 0.40);
            zone(-H,     -T, T, T * 2, QColor(240, 200, 0), 0.40);
            // Neutral-wall accent strips
            zone(-H, -H,            Field::SIZE_CM, T * 0.5, QColor(200, 200, 200), 0.22);
            zone(-H,  H - T * 0.5,  Field::SIZE_CM, T * 0.5, QColor(200, 200, 200), 0.22);
            // Stakes
            dot(0, 0, 18, QColor(150, 150, 150, 220), QColor(80, 80, 80));
            dot(0,  H - T * 0.5, 12, QColor(180, 180, 180, 220), QColor(110, 110, 110));
            dot(0, -(H - T * 0.5), 12, QColor(180, 180, 180, 220), QColor(110, 110, 110));
            dot( H - T * 0.5, 0, 12, QColor(50,  90, 210, 220), QColor(30,  60, 170));
            dot(-H + T * 0.5, 0, 12, QColor(200,  50,  50, 220), QColor(150,  30,  30));
            // Rings (180°-symmetric)
            auto gameRing = [&](double fx, double fy) {
                dot( fx,  fy, 8, QColor(240, 200, 0, 200), QColor(190, 150, 0));
                dot(-fx, -fy, 8, QColor(240, 200, 0, 200), QColor(190, 150, 0));
            };
            gameRing(T * 1.5, 0);  gameRing(0, T * 1.5);
            gameRing(T, T);        gameRing(T * 2, T);  gameRing(T, T * 2);
            // Mobile goal markers
            auto mgoal = [&](double fx, double fy, double sz) {
                double sx2 = fx - sz / 2, sy2 = -fy - sz / 2;
                auto* r = addRect(sx2, sy2, sz, sz,
                                  QPen(QColor(40, 40, 40), 1.5), QBrush(QColor(60, 60, 60, 210)));
                r->setZValue(3);
                m_templateItems.append(r);
            };
            mgoal( T * 2.0, 0, 14);  mgoal(-T * 2.0, 0, 14);
            mgoal(0, T * 2.0, 14);   mgoal(0, -T * 2.0, 14);
        }

        // ── Autonomous line (dashed vertical at x=0) ────────────────────────
        {
            QPen lp(QColor(255, 255, 255, hasImage ? 140 : 80), 1.5, Qt::DashLine);
            lp.setCosmetic(true);
            auto* l = addLine(0, -H, 0, H, lp);
            l->setZValue(2);
            m_templateItems.append(l);
        }
        return;
    }
}

void FieldScene::refreshPathItemFromModel(int idx) {
    if (idx >= 0 && idx < m_pathItems.size())
        m_pathItems[idx]->refreshFromModel();
}

void FieldScene::buildGrid() {
    for (auto* item : m_grid) { removeItem(item); delete item; }
    m_grid.clear();

    QPen dotPen(QColor(255, 255, 255, 80));
    dotPen.setWidthF(1.0);
    dotPen.setCosmetic(true);
    dotPen.setStyle(Qt::DotLine);

    QPen edgePen(QColor(255, 255, 255, 210));
    edgePen.setWidthF(2.0);
    edgePen.setCosmetic(true);

    const double h = Field::HALF_CM;
    const double t = Field::TILE_CM;

    for (int i = 0; i <= Field::TILES; ++i) {
        double pos    = -h + i * t;
        bool   isEdge = (i == 0 || i == Field::TILES);
        const QPen& pen = isEdge ? edgePen : dotPen;

        auto* vl = addLine(pos, -h, pos,  h, pen);
        auto* hl = addLine(-h, pos,  h, pos, pen);
        vl->setZValue(1);
        hl->setZValue(1);
        m_grid << vl << hl;
    }
}
