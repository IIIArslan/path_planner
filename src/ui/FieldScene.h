#pragma once
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QList>
#include "core/Project.h"
#include "graphics/BezierPathItem.h"
#include "graphics/RobotItem.h"

class FieldScene : public QGraphicsScene {
    Q_OBJECT
public:
    enum class EditMode { Select, DrawPath };

    explicit FieldScene(QObject* parent = nullptr);

    void setProject(Project* project);
    void loadFieldImage(const QString& path);
    void clearFieldImage();
    void setRobotVisible(bool v);
    void applyRobotConfig(const RobotConfig& cfg);

    // Call before any user-initiated edit to allow MainWindow to push an undo snapshot.
    void beginEdit();

    // Merge path b's segments into path a, then remove b.
    void mergePathsAt(int a, int b);

    // Start drawing a new path (switches to DrawPath mode).
    void startNewPath();
    // Finish the current drawing session and return to Select mode.
    void finishDrawing();
    // Cancel drawing and remove the partial path (if < 2 points were placed).
    void cancelDrawing();

    // Path management (called by PathListPanel)
    void removePath(int idx);
    void refreshPathItem(int idx);   // repaint after colour/visibility change
    void selectPathAt(int idx);      // select by index (−1 = deselect all)

    EditMode editMode()       const { return m_editMode;    }
    int      selectedPathIdx() const { return m_selectedIdx; }

    // Scene Y is positive-down; field Y is positive-up (north).
    static QPointF sceneToField(QPointF p) { return {p.x(), -p.y()}; }
    static QPointF fieldToScene(QPointF p)  { return {p.x(), -p.y()}; }

signals:
    void editModeChanged(EditMode mode);
    void pathCountChanged(int count);
    void pathSelectionChanged(int idx);
    void robotMoved(double x, double y, double headingDeg);
    void projectAboutToChange();  // emitted just before a user edit modifies the project

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
    void rebuildPathItems();   // re-creates all path items from m_project->paths

    Project*              m_project  = nullptr;
    QGraphicsRectItem*    m_bg       = nullptr;
    QGraphicsPixmapItem*  m_img      = nullptr;
    QList<QGraphicsItem*> m_grid;

    // Path items — parallel to m_project->paths
    QList<BezierPathItem*> m_pathItems;
    RobotItem* m_robotItem = nullptr;

    // Drawing state
    EditMode m_editMode      = EditMode::Select;
    int      m_drawPathIdx   = -1;   // index into m_project->paths
    bool     m_hasFirstPoint = false;
    Vec2     m_firstPoint    = {};
    int      m_selectedIdx   = -1;

    // Dashed preview line from last anchor to cursor
    QGraphicsLineItem* m_previewLine = nullptr;
};
