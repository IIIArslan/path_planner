#pragma once
#include <QMainWindow>
#include <QList>
#include <QByteArray>
#include "core/Project.h"
#include "ui/FieldScene.h"

class FieldView;
class RobotPanel;
class PathListPanel;
class OutputPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void buildMenuBar();
    void buildLayout();

    // Project lifecycle
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void refreshAllPanels();

    // Undo / redo
    void pushUndoState();
    void undo();
    void redo();
    void applyState(const QByteArray& json);

    // Theme
    void applyTheme(bool dark);

    void onEditModeChanged(FieldScene::EditMode mode);

    // ── data ──────────────────────────────────────────────────────────────
    Project*       m_project       = nullptr;
    FieldScene*    m_scene         = nullptr;
    FieldView*     m_fieldView     = nullptr;
    RobotPanel*    m_robotPanel    = nullptr;
    PathListPanel* m_pathListPanel = nullptr;
    OutputPanel*   m_outputPanel   = nullptr;

    QList<QByteArray> m_undoStack;
    QList<QByteArray> m_redoStack;
    QString           m_filePath;
    bool              m_isModified = false;
    bool              m_isDark     = true;
};
