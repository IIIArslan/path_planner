#include "MainWindow.h"
#include "FieldView.h"
#include "PathListPanel.h"
#include <QMenuBar>
#include <QAction>
#include <QSplitter>
#include <QLabel>
#include <QStatusBar>
#include <QFileDialog>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_project(new Project)
{
    setWindowTitle("VEX V5 Path Planner");
    resize(1280, 800);
    buildMenuBar();
    buildLayout();
}

MainWindow::~MainWindow() {
    delete m_project;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape &&
        m_scene->editMode() == FieldScene::EditMode::DrawPath) {
        m_scene->cancelDrawing();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

// ── menu bar ───────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    auto* file = menuBar()->addMenu("&File");
    file->addAction("&New Project",  this, &MainWindow::newProject,  QKeySequence::New);
    file->addAction("&Open...",      this, [](){},                    QKeySequence::Open);
    file->addSeparator();
    file->addAction("&Save",         this, [](){},                    QKeySequence::Save);
    file->addAction("Save &As...",   this, [](){},                    QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction("&Quit",         this, &QWidget::close,           QKeySequence::Quit);

    auto* field   = menuBar()->addMenu("&Field");
    auto* loadImg = field->addAction("Load Field Image...");
    connect(loadImg, &QAction::triggered, this, [this]() {
        QString p = QFileDialog::getOpenFileName(
            this, "Open Field Image", {}, "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!p.isEmpty()) m_scene->loadFieldImage(p);
    });
    field->addAction("Reset to Default", this, [this]() { m_scene->clearFieldImage(); });

    auto* view  = menuBar()->addMenu("&View");
    auto* dark  = view->addAction("Dark Mode");
    auto* light = view->addAction("Light Mode");
    dark->setCheckable(true);  dark->setChecked(true);
    light->setCheckable(true);
}

// ── layout ─────────────────────────────────────────────────────────────────

void MainWindow::buildLayout() {
    // ── Field scene + view ──────────────────────────────────────────────────
    m_scene     = new FieldScene(this);
    m_fieldView = new FieldView;
    m_fieldView->setScene(m_scene);
    m_scene->setProject(m_project);

    // ── Path list panel (left sidebar) ──────────────────────────────────────
    m_pathListPanel = new PathListPanel(m_project, m_scene, this);

    connect(m_scene, &FieldScene::editModeChanged, this, &MainWindow::onEditModeChanged);

    // ── Splitter ────────────────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(0);
    splitter->addWidget(m_pathListPanel);
    splitter->addWidget(m_fieldView);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);

    // ── Status bar ──────────────────────────────────────────────────────────
    statusBar()->addPermanentWidget(new QLabel("created by Kadir Arslan Ünal  "));
    statusBar()->setStyleSheet("color:#4a4b4f; font-size:10px;");
    statusBar()->showMessage("Scroll to zoom  ·  Middle-click to pan  ·  Click \"+ New Path\" to draw");
}

// ── slots ──────────────────────────────────────────────────────────────────

void MainWindow::newProject() {
    delete m_project;
    m_project = new Project;
    m_scene->setProject(m_project);
    m_pathListPanel->rebuild(); // Not called via signal here since setProject doesn't emit
    // Rebuild PathListPanel manually
    setWindowTitle("VEX V5 Path Planner — Untitled Project");
    statusBar()->showMessage("New project created.");
}

void MainWindow::onEditModeChanged(FieldScene::EditMode mode) {
    if (mode == FieldScene::EditMode::DrawPath) {
        statusBar()->showMessage(
            "Drawing mode: left-click to place points  ·  double-click or right-click to finish  ·  Esc to cancel"
        );
    } else {
        statusBar()->showMessage(
            "Select mode: click a path to edit its control points  ·  click \"+ New Path\" to draw"
        );
    }
}
