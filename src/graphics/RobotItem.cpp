#include "RobotItem.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QCursor>
#include <QPolygonF>

RobotItem::RobotItem(QGraphicsItem* parent)
    : QObject(nullptr), QGraphicsItem(parent)
{
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setZValue(10);
    setCursor(QCursor(Qt::SizeAllCursor));
}

void RobotItem::applyConfig(const RobotConfig& cfg) {
    m_w = cfg.widthCm;
    m_h = cfg.heightCm;
    m_suppressSignal = true;
    prepareGeometryChange();
    setPos(cfg.startX, -cfg.startY);   // field Y → scene Y (flip)
    setRotation(cfg.startHeading);     // 0=north, CW+, matches VEX convention
    m_suppressSignal = false;
}

QRectF RobotItem::boundingRect() const {
    return {-m_w / 2.0, -m_h / 2.0, m_w, m_h};
}

QPainterPath RobotItem::shape() const {
    QPainterPath p;
    p.addRect(boundingRect());
    return p;
}

void RobotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setRenderHint(QPainter::Antialiasing);

    // Body
    painter->setPen(QPen(QColor(100, 170, 255, 220), 1.5));
    painter->setBrush(QColor(60, 120, 220, 80));
    painter->drawRoundedRect(boundingRect(), 3.0, 3.0);

    // Forward arrow — local -Y = field north when heading=0
    double aw    = m_w * 0.30;
    double tipY  = -m_h / 2.0;
    double baseY = tipY + m_h * 0.25;
    QPolygonF arrow;
    arrow << QPointF(0,       tipY)
          << QPointF(-aw/2.0, baseY)
          << QPointF( aw/2.0, baseY);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(120, 200, 255, 210));
    painter->drawPolygon(arrow);
}

QVariant RobotItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged && !m_suppressSignal) {
        QPointF sp = value.toPointF();
        emit poseChanged(sp.x(), -sp.y(), rotation());  // scene → field Y-flip
    }
    return QGraphicsItem::itemChange(change, value);
}
