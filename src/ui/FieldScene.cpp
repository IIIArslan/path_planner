#include "FieldScene.h"
#include "core/FieldConstants.h"
#include <QPen>
#include <QBrush>
#include <QColor>
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
        m_pathItems[idx]->update();
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
        // Light mat background on top of the dark default
        {
            auto* mat = addRect(-H, -H, Field::SIZE_CM, Field::SIZE_CM,
                                QPen(Qt::NoPen), QBrush(QColor(215, 210, 200)));
            mat->setZValue(-1.5);
            m_templateItems.append(mat);
        }
        // Diagonal X lines corner-to-corner (field: SW↔NE and NW↔SE)
        {
            QPen xp(QColor(160, 158, 152, 230), 14.0);
            auto* l1 = addLine(-H,  H,  H, -H, xp);   // scene: BL→TR
            auto* l2 = addLine(-H, -H,  H,  H, xp);   // scene: TL→BR
            l1->setZValue(0.2);  l2->setZValue(0.2);
            m_templateItems << l1 << l2;
        }
        // Alliance corner zones (1×1 tile, 2 per alliance)
        zone(H - T,    -H, T, T, QColor(50,  90, 210), 0.55);   // Blue: top-right
        zone(H - T,  H-T,  T, T, QColor(50,  90, 210), 0.55);   // Blue: bottom-right
        zone(-H,      -H,  T, T, QColor(200,  50,  50), 0.55);   // Red:  top-left
        zone(-H,     H-T,  T, T, QColor(200,  50,  50), 0.55);   // Red:  bottom-left
        // Match-load zones (yellow bar, 1T wide × 2T tall, centred on alliance walls)
        zone(H - T,  -T, T, T * 2, QColor(240, 200, 0), 0.40);
        zone(-H,     -T, T, T * 2, QColor(240, 200, 0), 0.40);
        // Neutral-wall accent strips (±Y walls, 0.5T deep)
        zone(-H, -H,            Field::SIZE_CM, T * 0.5, QColor(200, 200, 200), 0.22);
        zone(-H,  H - T * 0.5,  Field::SIZE_CM, T * 0.5, QColor(200, 200, 200), 0.22);
        // Center high stake
        dot(0, 0, 18, QColor(150, 150, 150, 220), QColor(80, 80, 80));
        // Neutral-wall stakes (midpoints of ±Y walls, 0.5T inset)
        dot(0,  H - T * 0.5, 12, QColor(180, 180, 180, 220), QColor(110, 110, 110));
        dot(0, -(H - T * 0.5), 12, QColor(180, 180, 180, 220), QColor(110, 110, 110));
        // Alliance wall stakes (midpoints of ±X walls, 0.5T inset)
        dot( H - T * 0.5, 0, 12, QColor(50,  90, 210, 220), QColor(30,  60, 170));
        dot(-H + T * 0.5, 0, 12, QColor(200,  50,  50, 220), QColor(150,  30,  30));
        // Rings (yellow, 10 rings in 180°-symmetric layout)
        auto gameRing = [&](double fx, double fy) {
            dot( fx,  fy, 8, QColor(240, 200, 0, 200), QColor(190, 150, 0));
            dot(-fx, -fy, 8, QColor(240, 200, 0, 200), QColor(190, 150, 0));
        };
        gameRing(T * 1.5,   0);
        gameRing(0,       T * 1.5);
        gameRing(T,       T);
        gameRing(T * 2,   T);
        gameRing(T,       T * 2);
        // Mobile goal markers (dark squares, 4-way symmetric at 2T from centre)
        auto mgoal = [&](double fx, double fy, double sz) {
            double sx = fx - sz / 2, sy = -fy - sz / 2;
            auto* r = addRect(sx, sy, sz, sz,
                              QPen(QColor(40, 40, 40), 1.5), QBrush(QColor(60, 60, 60, 210)));
            r->setZValue(3);
            m_templateItems.append(r);
        };
        mgoal( T * 2.0,  0,       14);
        mgoal(-T * 2.0,  0,       14);
        mgoal(0,          T * 2.0, 14);
        mgoal(0,         -T * 2.0, 14);
        // Autonomous line (dashed vertical at x=0)
        {
            QPen lp(QColor(255, 255, 255, 80), 1.5, Qt::DashLine);
            lp.setCosmetic(true);
            auto* l = addLine(0, -H, 0, H, lp);
            l->setZValue(2);
            m_templateItems.append(l);
        }
        // Robot start positions
        if (type == 3) {  // Match: one robot per alliance, inside their corner zones
            auto sring = [&](double fx, double fy, QColor c) {
                double sx = fx, sy = -fy;
                auto* e = addEllipse(sx - T * 0.38, sy - T * 0.38, T * 0.76, T * 0.76,
                                     QPen(c, 2, Qt::DashLine), QBrush(Qt::NoBrush));
                e->setZValue(4);
                m_templateItems.append(e);
            };
            sring( H - T * 0.5,  H - T * 0.5, QColor(80,  110, 220));   // Blue top-right
            sring(-H + T * 0.5, -(H - T * 0.5), QColor(220,  80,  80)); // Red  bottom-left
        } else {  // Skills: single robot at Red corner (top-left in scene)
            double cx = -H + T * 0.5;
            double cy = -H + T * 0.5;   // scene y: top-left corner centre
            auto* e = addEllipse(cx - T * 0.38, cy - T * 0.38, T * 0.76, T * 0.76,
                                 QPen(QColor(255, 200, 0), 2, Qt::DashLine), QBrush(Qt::NoBrush));
            e->setZValue(4);
            m_templateItems.append(e);
            auto* txt = addSimpleText("START");
            txt->setBrush(QColor(255, 200, 0));
            txt->setPos(cx - 20, cy - 8);
            txt->setZValue(5);
            m_templateItems.append(txt);
        }
        return;
    }

    // ── VEX Over Under (2023-24) ─────────────────────────────────────────────

    // Red side: field y < 0 → scene y > 0 → rect top-left (−H, 0), size (SIZE, H)
    zone(-H, 0, Field::SIZE_CM, H, QColor(200, 50, 50));
    // Blue side: field y > 0 → scene y < 0 → rect top-left (−H, −H), size (SIZE, H)
    zone(-H, -H, Field::SIZE_CM, H, QColor(50, 90, 210));

    // Offensive zones (2×1 tile strips near the center, opposite diagonal)
    // Red offensive (bottom-right of centre): field x∈[0,2T], y∈[−T,0]
    //   → scene x∈[0,2T], y∈[0,T]   → rect(0, 0, 2T, T)
    zone(0,    0,   T * 2, T, QColor(200, 50, 50),  0.35);
    // Blue offensive (top-left of centre): field x∈[−2T,0], y∈[0,T]
    //   → scene x∈[−2T,0], y∈[−T,0] → rect(−2T, −T, 2T, T)
    zone(-T * 2, -T, T * 2, T, QColor(50, 90, 210), 0.35);

    // Match-load zones (1×1 tile, scene):
    // Red load – field bottom-left tile: x∈[−H,−H+T], y∈[−H,−H+T] → scene y∈[H−T,H]
    zone(-H, H - T, T, T, QColor(200, 50, 50),  0.50);
    // Blue load – field top-right tile: x∈[H−T,H], y∈[H−T,H] → scene y∈[−H,−H+T]
    zone(H - T, -H, T, T, QColor(50, 90, 210), 0.50);

    // Center barrier (full-width horizontal strip at y=0 in scene)
    {
        auto* bar = addRect(-H, -5, Field::SIZE_CM, 10,
                            QPen(Qt::NoPen), QBrush(QColor(160, 160, 160, 210)));
        bar->setZValue(2);
        m_templateItems.append(bar);
    }

    // Autonomous line (vertical, at x=0)
    {
        QPen lp(QColor(255, 255, 255, 70), 1.5, Qt::DashLine);
        lp.setCosmetic(true);
        auto* l = addLine(0, -H, 0, H, lp);
        l->setZValue(2);
        m_templateItems.append(l);
    }

    // Goals (2 large cylinders, one per half, offset from center barrier)
    dot( T,      -T * 0.65, 14, QColor(200, 50, 50,  150), QColor(150, 30, 30));
    dot(-T,       T * 0.65, 14, QColor(50, 90, 210, 150), QColor(30, 60, 170));

    // Triball starting positions (small filled circles)
    // Red side (~3 positions)
    dot(-T * 1.5, -T * 0.5, 7, QColor(220, 60, 60, 180),  QColor(160, 30, 30));
    dot(-T * 0.5, -T * 0.5, 7, QColor(220, 60, 60, 180),  QColor(160, 30, 30));
    dot( T * 0.5, -T * 1.5, 7, QColor(220, 60, 60, 180),  QColor(160, 30, 30));
    // Blue side
    dot( T * 1.5,  T * 0.5, 7, QColor(60, 100, 220, 180), QColor(30, 60, 170));
    dot( T * 0.5,  T * 0.5, 7, QColor(60, 100, 220, 180), QColor(30, 60, 170));
    dot(-T * 0.5,  T * 1.5, 7, QColor(60, 100, 220, 180), QColor(30, 60, 170));

    // Robot starting positions
    if (type == 1) {
        // Match – 2 robots per alliance (bottom row = red, top row = blue)
        for (double sign : {-1.0, 1.0}) {
            // Alliance two tiles from the corner
            double fx1 = -H + T * 0.5, fy1 = sign * (H - T * 0.5);
            double fx2 = -H + T * 1.5, fy2 = sign * (H - T * 0.5);
            QColor c = (sign < 0) ? QColor(220, 80, 80) : QColor(80, 110, 220);
            auto ring = [&](double fx, double fy) {
                double sx = fx, sy = -fy;
                auto* e = addEllipse(sx - T * 0.38, sy - T * 0.38, T * 0.76, T * 0.76,
                                     QPen(c, 2, Qt::DashLine), QBrush(Qt::NoBrush));
                e->setZValue(4);
                m_templateItems.append(e);
            };
            ring(fx1, fy1);
            ring(fx2, fy2);
        }
    } else {
        // Skills – single robot, starts at red match-load zone corner tile
        double cx = -H + T * 0.5, cy = -(H - T * 0.5);  // scene coords
        auto* e = addEllipse(cx - T * 0.38, cy - T * 0.38, T * 0.76, T * 0.76,
                             QPen(QColor(255, 200, 0), 2, Qt::DashLine), QBrush(Qt::NoBrush));
        e->setZValue(4);
        m_templateItems.append(e);
        auto* t = addSimpleText("START");
        t->setBrush(QColor(255, 200, 0));
        t->setPos(cx - 20, cy - 8);
        t->setZValue(5);
        m_templateItems.append(t);
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
