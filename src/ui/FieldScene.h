#pragma once
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QList>
#include "core/Project.h"

class FieldScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit FieldScene(QObject* parent = nullptr);

    void setProject(Project* project);
    void loadFieldImage(const QString& path);
    void clearFieldImage();

    // Scene Y is positive-down; field Y is positive-up (north).
    // These two operations are identical (sign flip on Y) but named for clarity.
    static QPointF sceneToField(QPointF p) { return {p.x(), -p.y()}; }
    static QPointF fieldToScene(QPointF p)  { return {p.x(), -p.y()}; }

private:
    void buildBackground();
    void buildGrid();

    Project*              m_project = nullptr;
    QGraphicsRectItem*    m_bg      = nullptr;
    QGraphicsPixmapItem*  m_img     = nullptr;
    QList<QGraphicsItem*> m_grid;
};
