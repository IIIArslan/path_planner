#pragma once
#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include "core/Project.h"
#include "ui/FieldScene.h"
#include "io/PythonExporter.h"

// Bottom panel that generates and displays Python output for a selected path.
// Two read-only text areas (waypoints and headings) each have a Copy button.
class OutputPanel : public QWidget {
    Q_OBJECT
public:
    explicit OutputPanel(Project* project, FieldScene* scene, QWidget* parent = nullptr);

    void setProject(Project* p);
    void refreshPathList();
    void refreshTheme(bool dark);

private:
    void generate();
    void generateAll();
    PythonExporter::Format selectedFormat() const;

    static QTextEdit*  makeTextArea(QWidget* parent);
    static QPushButton* makeCopyBtn(QTextEdit* target, QWidget* parent);

    Project*    m_project;
    FieldScene* m_scene;

    QComboBox*      m_pathCombo  = nullptr;
    QComboBox*      m_fmtCombo   = nullptr;
    QDoubleSpinBox* m_stepSpin   = nullptr;
    QTextEdit*      m_wpText     = nullptr;
    QTextEdit*      m_hdText     = nullptr;
    QLabel*         m_statusLbl  = nullptr;
    QPushButton*    m_genBtn     = nullptr;
};
