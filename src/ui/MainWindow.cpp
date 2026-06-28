#include "MainWindow.h"
#include "FieldScene.h"
#include "FieldView.h"
#include <QMenuBar>
#include <QAction>
#include <QSplitter>
#include <QLabel>
#include <QFrame>
#include <QStatusBar>
#include <QFileDialog>

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

void MainWindow::buildMenuBar() {
    auto* file = menuBar()->addMenu("&File");
    file->addAction("&New Project",  this, &MainWindow::newProject,  QKeySequence::New);
    file->addAction("&Open...",      this, [](){},                    QKeySequence::Open);
    file->addSeparator();
    file->addAction("&Save",         this, [](){},                    QKeySequence::Save);
    file->addAction("Save &As...",   this, [](){},                    QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction("&Quit",         this, &QWidget::close,           QKeySequence::Quit);

    auto* field = menuBar()->addMenu("&Field");

    auto* loadImg = field->addAction("Load Field Image...");
    connect(loadImg, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Open Field Image", {},
            "Images (*.png *.jpg *.jpeg *.bmp)"
        );
        if (!path.isEmpty()) m_scene->loadFieldImage(path);
    });

    auto* clearImg = field->addAction("Reset to Default");
    connect(clearImg, &QAction::triggered, this, [this]() {
        m_scene->clearFieldImage();
    });

    auto* view = menuBar()->addMenu("&View");
    auto* dark  = view->addAction("Dark Mode");
    auto* light = view->addAction("Light Mode");
    dark->setCheckable(true);
    light->setCheckable(true);
    dark->setChecked(true);
}

void MainWindow::buildLayout() {
    // ── Left panel (placeholder — Phase 4 will replace with PathListPanel) ─
    auto* leftPanel = new QFrame;
    leftPanel->setFrameShape(QFrame::NoFrame);
    leftPanel->setFixedWidth(220);
    leftPanel->setStyleSheet("background:#1a1b1e; border-right:1px solid #2c2d31;");

    auto* pathsLabel = new QLabel("Paths", leftPanel);
    pathsLabel->setContentsMargins(14, 14, 0, 0);
    pathsLabel->setStyleSheet("color:#7a7b7f; font-weight:600; font-size:12px; letter-spacing:0.5px;");

    // ── Field canvas ────────────────────────────────────────────────────────
    m_scene     = new FieldScene(this);
    m_fieldView = new FieldView;
    m_fieldView->setScene(m_scene);
    m_scene->setProject(m_project);

    // ── Splitter ────────────────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(0);
    splitter->addWidget(leftPanel);
    splitter->addWidget(m_fieldView);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);

    // ── Status bar ──────────────────────────────────────────────────────────
    statusBar()->addPermanentWidget(
        new QLabel("created by Kadir Arslan Ünal  ")
    );
    statusBar()->setStyleSheet("color:#4a4b4f; font-size:10px;");
    statusBar()->showMessage(
        "Scroll to zoom  ·  Middle-click + drag to pan"
    );
}

void MainWindow::newProject() {
    delete m_project;
    m_project = new Project;
    m_scene->setProject(m_project);
    setWindowTitle("VEX V5 Path Planner — Untitled Project");
    statusBar()->showMessage("New project created.");
}
