#include "ControlPointItem.h"
#include <QPen>
#include <QBrush>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>

ControlPointItem::ControlPointItem(Role role, QGraphicsItem* parent)
    : QObject(nullptr)
    , QGraphicsEllipseItem(parent)
    , m_role(role)
{
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setFlag(ItemIgnoresTransformations); // always drawn at fixed screen size
    setCursor(QCursor(Qt::SizeAllCursor));

    if (role == Role::Anchor) {
        setRect(-6.0, -6.0, 12.0, 12.0);
        setBrush(Qt::white);
        setPen(QPen(QColor(55, 55, 60), 1.5));
    } else {
        setRect(-4.5, -4.5, 9.0, 9.0);
        setBrush(QColor(66, 133, 244));
        setPen(QPen(QColor(30, 90, 200), 1.2));
    }

    setZValue(20);
}

void ControlPointItem::setPathColor(QColor c) {
    if (m_role == Role::Anchor) {
        setBrush(Qt::white);
        setPen(QPen(c, 1.5));
    } else {
        setBrush(c.lighter(140));
        setPen(QPen(c, 1.2));
    }
}

void ControlPointItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    emit dragStarted();
    QGraphicsEllipseItem::mousePressEvent(event);
}

QVariant ControlPointItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged)
        emit positionChanged(value.toPointF());
    return QGraphicsEllipseItem::itemChange(change, value);
}
