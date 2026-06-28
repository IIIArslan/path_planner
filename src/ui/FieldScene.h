#pragma once
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QList>
#include "core/Project.h"
#include "graphics/BezierPathItem.h"

class FieldScene : public QGraphicsScene {
    Q_OBJECT
public:
    enum class EditMode { Select, DrawPath };

    explicit FieldScene(QObject* parent = nullptr);

    void setProject(Project* project);
    void loadFieldImage(const QString& path);
    void clearFieldImage();

    // Start drawing a new path (switches to DrawPath mode).
    void startNewPath();
    // Finish the current drawing session and return to Select mode.
    void finishDrawing();
    // Cancel drawing and remove the partial path (if < 2 points were placed).
    void cancelDrawing();

    EditMode editMode() const { return m_editMode; }

    // Scene Y is positive-down; field Y is positive-up (north).
    static QPointF sceneToField(QPointF p) { return {p.x(), -p.y()}; }
    static QPointF fieldToScene(QPointF p)  { return {p.x(), -p.y()}; }

signals:
    void editModeChanged(EditMode mode);
    void pathCountChanged(int count);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void buildBackground();
    void buildGrid();
    void addPointToPath(QPointF fieldPos);
    BezierPathItem* createPathItem(Path* path);
    void selectPath(BezierPathItem* item);

    Project*              m_project  = nullptr;
    QGraphicsRectItem*    m_bg       = nullptr;
    QGraphicsPixmapItem*  m_img      = nullptr;
    QList<QGraphicsItem*> m_grid;

    // Path items — parallel to m_project->paths
    QList<BezierPathItem*> m_pathItems;

    // Drawing state
    EditMode m_editMode      = EditMode::Select;
    int      m_drawPathIdx   = -1;   // index into m_project->paths
    bool     m_hasFirstPoint = false;
    Vec2     m_firstPoint    = {};

    // Dashed preview line from last anchor to cursor
    QGraphicsLineItem* m_previewLine = nullptr;
};
