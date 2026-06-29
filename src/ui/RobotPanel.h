#pragma once
#include <QWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include "core/Project.h"
#include "ui/FieldScene.h"

class RobotPanel : public QWidget {
    Q_OBJECT
public:
    explicit RobotPanel(Project* project, FieldScene* scene, QWidget* parent = nullptr);

    void setProject(Project* p);
    void refreshTheme(bool dark);

private:
    void onRobotMoved(double x, double y, double heading);
    void pushToScene();

    Project*    m_project;
    FieldScene* m_scene;

    QCheckBox*      m_visCheck     = nullptr;
    QDoubleSpinBox* m_xSpin        = nullptr;
    QDoubleSpinBox* m_ySpin        = nullptr;
    QDoubleSpinBox* m_hdgSpin      = nullptr;
    QDoubleSpinBox* m_wSpin        = nullptr;
    QDoubleSpinBox* m_hSpin        = nullptr;

    // Velocity profile
    QDoubleSpinBox* m_kCurveSpin    = nullptr;
    QDoubleSpinBox* m_vMinSpin      = nullptr;
    QDoubleSpinBox* m_aMaxSpin      = nullptr;
    QDoubleSpinBox* m_lookAheadSpin = nullptr;
    QDoubleSpinBox* m_vEndSpin      = nullptr;
};
