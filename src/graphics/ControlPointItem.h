#pragma once
#include <QObject>
#include <QGraphicsEllipseItem>

// A single draggable control point rendered at fixed screen size (ItemIgnoresTransformations).
// Emits positionChanged(scenePos) whenever it is dragged.
class ControlPointItem : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
public:
    enum class Role { Anchor, Handle };

    explicit ControlPointItem(Role role, QGraphicsItem* parent = nullptr);
    Role role() const { return m_role; }

signals:
    void positionChanged(QPointF newScenePos);
    void dragStarted();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    Role m_role;
};
