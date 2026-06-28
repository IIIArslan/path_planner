#pragma once
#include <QGraphicsObject>
#include <QPainterPath>
#include <QList>
#include "core/Path.h"

class ControlPointItem;

// Renders one Path (piecewise cubic Bezier) and owns its ControlPointItems.
//
// Coordinate convention: field Y is up (+north), scene Y is down.
// All conversions go through the inline fs()/sf() helpers.
//
// G1 continuity: when a handle is dragged the opposite handle at the shared
// anchor is mirrored (preserving the original arm length).
class BezierPathItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit BezierPathItem(Path* path, QGraphicsItem* parent = nullptr);

    Path* path() const { return m_path; }

    // Re-read control point positions from the Path model and rebuild items.
    void refreshFromModel();

    // Sync control-point colours and visibility after a colour or visible change.
    void syncFromModel();

    // Show / hide control-point handles (only visible when this path is selected).
    void setEditSelected(bool sel);
    bool editSelected() const { return m_editSelected; }

    QRectF      boundingRect() const override;
    QPainterPath shape()       const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

signals:
    void pathClicked(BezierPathItem* self);
    void aboutToEdit();  // fires when any control point starts being dragged

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void buildItems();
    void clearItems();
    void updateCurve();
    void applyColors();

    void onAnchorMoved(int anchorIdx, QPointF scenePos);
    void onHandleMoved(int segIdx, bool isP1, QPointF scenePos);

    // Field ↔ scene coordinate helpers (sign flip on Y)
    static QPointF fs(Vec2 v)     { return {v.x, -v.y}; }
    static Vec2    sf(QPointF p)  { return {p.x(), -p.y()}; }

    Path* m_path;
    bool  m_editSelected = false;

    QPainterPath m_curvePath;

    QList<ControlPointItem*>                m_anchors;    // N+1 for N segments
    struct SegHandles { ControlPointItem* p1; ControlPointItem* p2; };
    QList<SegHandles>                       m_segHandles; // N entries
};
