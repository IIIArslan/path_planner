#include "BezierPathItem.h"
#include "ControlPointItem.h"
#include <QPainter>
#include <QPen>
#include <QPainterPathStroker>
#include <QGraphicsSceneMouseEvent>
#include <cmath>

BezierPathItem::BezierPathItem(Path* path, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_path(path)
{
    setFlag(ItemIsSelectable, false); // selection managed manually via setEditSelected
    setAcceptHoverEvents(true);
    setZValue(5);
    refreshFromModel();
}

QRectF BezierPathItem::boundingRect() const {
    // Fixed rect covering the whole field (365.76 × 365.76 cm) plus handle margin
    return QRectF(-220.0, -220.0, 440.0, 440.0);
}

QPainterPath BezierPathItem::shape() const {
    QPainterPathStroker s;
    s.setWidth(8.0); // 8 cm click tolerance around the curve
    return s.createStroke(m_curvePath);
}

// ── public ─────────────────────────────────────────────────────────────────

void BezierPathItem::refreshFromModel() {
    clearItems();
    if (!m_path->isEmpty()) {
        buildItems();
        updateCurve();
    }
    update();
}

void BezierPathItem::setEditSelected(bool sel) {
    m_editSelected = sel;
    for (auto* a  : m_anchors)    a->setVisible(sel);
    for (auto& sh : m_segHandles) { sh.p1->setVisible(sel); sh.p2->setVisible(sel); }
    update();
}

// ── paint ──────────────────────────────────────────────────────────────────

void BezierPathItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    if (m_path->isEmpty()) return;

    painter->setRenderHint(QPainter::Antialiasing);

    // Handle arm lines (dashed): only when edit-selected
    if (m_editSelected) {
        QPen armPen(QColor(170, 170, 180, 130), 1.0, Qt::DashLine);
        armPen.setCosmetic(true);
        painter->setPen(armPen);
        for (const auto& seg : m_path->segments) {
            painter->drawLine(fs(seg.p0), fs(seg.p1));
            painter->drawLine(fs(seg.p3), fs(seg.p2));
        }
    }

    // Bezier curve
    const auto& c = m_path->color;
    int alpha = m_path->visible ? 215 : 60;
    QPen curvePen(QColor(c.r, c.g, c.b, alpha), 2.5);
    curvePen.setCosmetic(true);
    curvePen.setCapStyle(Qt::RoundCap);
    curvePen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(curvePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_curvePath);
}

// ── private: item building ─────────────────────────────────────────────────

void BezierPathItem::buildItems() {
    int N = static_cast<int>(m_path->segments.size());

    // Anchor items (N+1)
    for (int i = 0; i <= N; ++i) {
        Vec2 fp = (i == 0) ? m_path->segments[0].p0
                            : m_path->segments[i - 1].p3;

        auto* a = new ControlPointItem(ControlPointItem::Role::Anchor, this);
        a->setPos(fs(fp));
        a->setVisible(m_editSelected);

        int idx = i;
        connect(a, &ControlPointItem::positionChanged, this,
            [this, idx](QPointF sp) { onAnchorMoved(idx, sp); });
        connect(a, &ControlPointItem::dragStarted, this, [this]{ emit aboutToEdit(); });
        m_anchors.append(a);
    }

    // Handle items (2 per segment)
    for (int i = 0; i < N; ++i) {
        const auto& seg = m_path->segments[i];

        auto* h1 = new ControlPointItem(ControlPointItem::Role::Handle, this);
        h1->setPos(fs(seg.p1));
        h1->setVisible(m_editSelected);
        int si = i;
        connect(h1, &ControlPointItem::positionChanged, this,
            [this, si](QPointF sp) { onHandleMoved(si, true, sp); });
        connect(h1, &ControlPointItem::dragStarted, this, [this]{ emit aboutToEdit(); });

        auto* h2 = new ControlPointItem(ControlPointItem::Role::Handle, this);
        h2->setPos(fs(seg.p2));
        h2->setVisible(m_editSelected);
        connect(h2, &ControlPointItem::positionChanged, this,
            [this, si](QPointF sp) { onHandleMoved(si, false, sp); });
        connect(h2, &ControlPointItem::dragStarted, this, [this]{ emit aboutToEdit(); });

        m_segHandles.append(SegHandles{h1, h2});
    }
}

void BezierPathItem::clearItems() {
    for (auto* a  : m_anchors)    delete a;
    for (auto& sh : m_segHandles) { delete sh.p1; delete sh.p2; }
    m_anchors.clear();
    m_segHandles.clear();
    m_curvePath = QPainterPath();
}

void BezierPathItem::updateCurve() {
    m_curvePath = QPainterPath();
    if (m_path->isEmpty()) return;

    m_curvePath.moveTo(fs(m_path->segments.front().p0));
    for (const auto& seg : m_path->segments)
        m_curvePath.cubicTo(fs(seg.p1), fs(seg.p2), fs(seg.p3));

    update();
}

// ── private: control-point callbacks ──────────────────────────────────────

void BezierPathItem::onAnchorMoved(int anchorIdx, QPointF scenePos) {
    Vec2 newFP = sf(scenePos);
    int  N     = static_cast<int>(m_path->segments.size());

    if (anchorIdx == 0) {
        Vec2 delta = newFP - m_path->segments[0].p0;
        m_path->segments[0].p0 = newFP;
        m_path->segments[0].p1 = m_path->segments[0].p1 + delta;
        m_segHandles[0].p1->setPos(fs(m_path->segments[0].p1));

    } else if (anchorIdx == N) {
        Vec2 delta = newFP - m_path->segments[N - 1].p3;
        m_path->segments[N - 1].p3 = newFP;
        m_path->segments[N - 1].p2 = m_path->segments[N - 1].p2 + delta;
        m_segHandles[N - 1].p2->setPos(fs(m_path->segments[N - 1].p2));

    } else {
        // Interior anchor: translate its two attached handles
        Vec2 delta = newFP - m_path->segments[anchorIdx - 1].p3;

        m_path->segments[anchorIdx - 1].p3 = newFP;
        m_path->segments[anchorIdx].p0     = newFP;

        m_path->segments[anchorIdx - 1].p2 = m_path->segments[anchorIdx - 1].p2 + delta;
        m_segHandles[anchorIdx - 1].p2->setPos(fs(m_path->segments[anchorIdx - 1].p2));

        m_path->segments[anchorIdx].p1 = m_path->segments[anchorIdx].p1 + delta;
        m_segHandles[anchorIdx].p1->setPos(fs(m_path->segments[anchorIdx].p1));
    }

    updateCurve();
}

void BezierPathItem::onHandleMoved(int segIdx, bool isP1, QPointF scenePos) {
    Vec2 newFP = sf(scenePos);
    auto& seg  = m_path->segments[segIdx];
    int   N    = static_cast<int>(m_path->segments.size());

    if (isP1) {
        seg.p1 = newFP;
        // G1: mirror p2 of previous segment through the shared anchor (seg.p0)
        if (segIdx > 0) {
            Vec2   anchor = seg.p0;
            Vec2   old_p2 = m_path->segments[segIdx - 1].p2;
            double len    = (old_p2 - anchor).length();
            Vec2   dir    = anchor - newFP;
            double dlen   = dir.length();
            if (dlen > 1e-6) {
                Vec2 mirror = anchor + dir * (len / dlen);
                m_path->segments[segIdx - 1].p2 = mirror;
                // Block signal to avoid recursive update
                QSignalBlocker block(m_segHandles[segIdx - 1].p2);
                m_segHandles[segIdx - 1].p2->setPos(fs(mirror));
            }
        }
    } else {
        seg.p2 = newFP;
        // G1: mirror p1 of next segment through the shared anchor (seg.p3)
        if (segIdx < N - 1) {
            Vec2   anchor = seg.p3;
            Vec2   old_p1 = m_path->segments[segIdx + 1].p1;
            double len    = (old_p1 - anchor).length();
            Vec2   dir    = anchor - newFP;
            double dlen   = dir.length();
            if (dlen > 1e-6) {
                Vec2 mirror = anchor + dir * (len / dlen);
                m_path->segments[segIdx + 1].p1 = mirror;
                QSignalBlocker block(m_segHandles[segIdx + 1].p1);
                m_segHandles[segIdx + 1].p1->setPos(fs(mirror));
            }
        }
    }

    updateCurve();
}

void BezierPathItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    emit pathClicked(this);
    QGraphicsObject::mousePressEvent(event);
}
