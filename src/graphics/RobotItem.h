#pragma once
#include <QObject>
#include <QGraphicsItem>
#include <QPainterPath>
#include "core/RobotConfig.h"

class RobotItem : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit RobotItem(QGraphicsItem* parent = nullptr);
    void applyConfig(const RobotConfig& cfg);

    QRectF       boundingRect() const override;
    QPainterPath shape() const override;
    void         paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

signals:
    void poseChanged(double fieldX, double fieldY, double headingDeg);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    double m_w = 38.0, m_h = 38.0;
    bool   m_suppressSignal = false;
};
