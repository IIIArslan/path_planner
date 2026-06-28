#include "MainWindow.h"
#include "FieldView.h"
#include <QMenuBar>
#include <QAction>
#include <QSplitter>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
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
    if (event->key() == Qt::Key_Escape && m_scene->editMode() == FieldScene::EditMode::DrawPath) {
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

    auto* field    = menuBar()->addMenu("&Field");
    auto* loadImg  = field->addAction("Load Field Image...");
    connect(loadImg, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Open Field Image", {}, "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!path.isEmpty()) m_scene->loadFieldImage(path);
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
    // ── Left panel ──────────────────────────────────────────────────────────
    auto* leftPanel = new QFrame;
    leftPanel->setFrameShape(QFrame::NoFrame);
    leftPanel->setFixedWidth(220);
    leftPanel->setStyleSheet("background:#1a1b1e; border-right:1px solid #2c2d31;");

    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // Panel header
    auto* header = new QLabel("Paths", leftPanel);
    header->setContentsMargins(14, 14, 14, 10);
    header->setStyleSheet("color:#7a7b7f; font-weight:600; font-size:12px; letter-spacing:0.5px;");
    leftLayout->addWidget(header);

    // "+" add path button
    m_addPathBtn = new QPushButton("+ New Path", leftPanel);
    m_addPathBtn->setStyleSheet(
        "QPushButton {"
        "  background:#2a2b2f; color:#c8c9cd;"
        "  border:1px solid #3a3b3f; border-radius:5px;"
        "  padding:6px 12px; margin:0 10px 10px 10px;"
        "  font-size:12px;"
        "}"
        "QPushButton:hover { background:#3a3b3f; }"
        "QPushButton:pressed { background:#444549; }"
    );
    connect(m_addPathBtn, &QPushButton::clicked, this, [this]() {
        m_scene->startNewPath();
    });
    leftLayout->addWidget(m_addPathBtn);

    leftLayout->addStretch();

    // ── Field canvas ────────────────────────────────────────────────────────
    m_scene     = new FieldScene(this);
    m_fieldView = new FieldView;
    m_fieldView->setScene(m_scene);
    m_scene->setProject(m_project);

    connect(m_scene, &FieldScene::editModeChanged, this, &MainWindow::onEditModeChanged);

    // ── Splitter ────────────────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(0);
    splitter->addWidget(leftPanel);
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
    setWindowTitle("VEX V5 Path Planner — Untitled Project");
    statusBar()->showMessage("New project created.");
}

void MainWindow::onEditModeChanged(FieldScene::EditMode mode) {
    if (mode == FieldScene::EditMode::DrawPath) {
        m_addPathBtn->setText("Finish (Esc / Right-click)");
        m_addPathBtn->setStyleSheet(
            "QPushButton {"
            "  background:#1a3a2a; color:#4caf80;"
            "  border:1px solid #2a5a3a; border-radius:5px;"
            "  padding:6px 12px; margin:0 10px 10px 10px;"
            "  font-size:12px;"
            "}"
            "QPushButton:hover { background:#2a4a3a; }"
        );
        statusBar()->showMessage("Drawing mode: left-click to add points  ·  double-click or right-click to finish  ·  Esc to cancel");
    } else {
        m_addPathBtn->setText("+ New Path");
        m_addPathBtn->setStyleSheet(
            "QPushButton {"
            "  background:#2a2b2f; color:#c8c9cd;"
            "  border:1px solid #3a3b3f; border-radius:5px;"
            "  padding:6px 12px; margin:0 10px 10px 10px;"
            "  font-size:12px;"
            "}"
            "QPushButton:hover { background:#3a3b3f; }"
            "QPushButton:pressed { background:#444549; }"
        );
        statusBar()->showMessage("Select mode: click a path to edit its control points");
    }
}
