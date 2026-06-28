#pragma once
#include <QMainWindow>
#include "core/Project.h"

class FieldScene;
class FieldView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void buildMenuBar();
    void buildLayout();
    void newProject();

    Project*    m_project   = nullptr;
    FieldScene* m_scene     = nullptr;
    FieldView*  m_fieldView = nullptr;
};
