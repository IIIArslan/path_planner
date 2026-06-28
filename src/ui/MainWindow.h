#pragma once
#include <QMainWindow>
#include "core/Project.h"
#include "ui/FieldScene.h"

class FieldView;
class PathListPanel;
class OutputPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildMenuBar();
    void buildLayout();
    void newProject();
    void onEditModeChanged(FieldScene::EditMode mode);

    Project*       m_project       = nullptr;
    FieldScene*    m_scene         = nullptr;
    FieldView*     m_fieldView     = nullptr;
    PathListPanel* m_pathListPanel = nullptr;
    OutputPanel*   m_outputPanel   = nullptr;
};
