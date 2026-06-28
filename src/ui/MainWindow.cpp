#include "MainWindow.h"
#include "FieldView.h"
#include "RobotPanel.h"
#include "PathListPanel.h"
#include "OutputPanel.h"
#include "io/ProjectSerializer.h"
#include <QMenuBar>
#include <QAction>
#include <QActionGroup>
#include <QSplitter>
#include <QFrame>
#include <QLabel>
#include <QStatusBar>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QApplication>
#include <QPalette>
#include <QShortcut>

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

// ── events ─────────────────────────────────────────────────────────────────

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape &&
        m_scene->editMode() == FieldScene::EditMode::DrawPath) {
        m_scene->cancelDrawing();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_isModified) { event->accept(); return; }
    auto reply = QMessageBox::question(
        this, "Unsaved Changes",
        "The project has unsaved changes.",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );
    if (reply == QMessageBox::Save) {
        saveProject();
        event->accept();
    } else if (reply == QMessageBox::Discard) {
        event->accept();
    } else {
        event->ignore();
    }
}

// ── menu bar ───────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    // File
    auto* file = menuBar()->addMenu("&File");
    file->addAction("&New Project",  this, &MainWindow::newProject,    QKeySequence::New);
    file->addAction("&Open...",      this, &MainWindow::openProject,    QKeySequence::Open);
    file->addSeparator();
    file->addAction("&Save",         this, &MainWindow::saveProject,    QKeySequence::Save);
    file->addAction("Save &As...",   this, &MainWindow::saveProjectAs,  QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction("&Quit",         this, &QWidget::close,             QKeySequence::Quit);

    // Edit
    auto* edit = menuBar()->addMenu("&Edit");
    edit->addAction("Undo", this, &MainWindow::undo, QKeySequence::Undo);
    edit->addAction("Redo", this, &MainWindow::redo, QKeySequence::Redo);

    // Field
    auto* field   = menuBar()->addMenu("&Field");
    field->addAction("Over Under — Match",  this, [this]() { m_scene->applyFieldTemplate(1); });
    field->addAction("Over Under — Skills", this, [this]() { m_scene->applyFieldTemplate(2); });
    field->addAction("Clear Field Template", this, [this]() { m_scene->applyFieldTemplate(0); });
    field->addSeparator();
    auto* loadImg = field->addAction("Load Custom Field Image...");
    connect(loadImg, &QAction::triggered, this, [this]() {
        QString p = QFileDialog::getOpenFileName(
            this, "Open Field Image", {}, "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!p.isEmpty()) m_scene->loadFieldImage(p);
    });
    field->addAction("Reset Field Image", this, [this]() { m_scene->clearFieldImage(); });

    // View
    auto* view       = menuBar()->addMenu("&View");
    auto* themeGroup = new QActionGroup(this);
    auto* dark       = view->addAction("Dark Mode");
    auto* light      = view->addAction("Light Mode");
    dark->setCheckable(true);  dark->setChecked(true);
    light->setCheckable(true);
    themeGroup->addAction(dark);
    themeGroup->addAction(light);
    connect(dark,  &QAction::triggered, this, [this]{ applyTheme(true);  });
    connect(light, &QAction::triggered, this, [this]{ applyTheme(false); });
}

// ── layout ─────────────────────────────────────────────────────────────────

void MainWindow::buildLayout() {
    m_scene     = new FieldScene(this);
    m_fieldView = new FieldView;
    m_fieldView->setScene(m_scene);
    m_scene->setProject(m_project);

    // Push undo snapshot before every user edit
    connect(m_scene, &FieldScene::projectAboutToChange, this, &MainWindow::pushUndoState);
    connect(m_scene, &FieldScene::editModeChanged, this, &MainWindow::onEditModeChanged);

    // Escape always cancels drawing regardless of which widget has focus
    auto* escShortcut = new QShortcut(Qt::Key_Escape, this);
    escShortcut->setContext(Qt::ApplicationShortcut);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (m_scene->editMode() == FieldScene::EditMode::DrawPath)
            m_scene->cancelDrawing();
    });

    // Left sidebar
    m_robotPanel    = new RobotPanel(m_project, m_scene, this);
    m_pathListPanel = new PathListPanel(m_project, m_scene, this);

    auto* leftWidget = new QWidget(this);
    leftWidget->setFixedWidth(220);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    leftLayout->addWidget(m_robotPanel);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#252630;");
    leftLayout->addWidget(sep);
    leftLayout->addWidget(m_pathListPanel, 1);

    // Output panel
    m_outputPanel = new OutputPanel(m_project, m_scene, this);

    // Right: field top, output bottom
    auto* vSplitter = new QSplitter(Qt::Vertical);
    vSplitter->addWidget(m_fieldView);
    vSplitter->addWidget(m_outputPanel);
    vSplitter->setStretchFactor(0, 1);
    vSplitter->setStretchFactor(1, 0);
    vSplitter->setSizes({580, 200});

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(0);
    splitter->addWidget(leftWidget);
    splitter->addWidget(vSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);

    statusBar()->addPermanentWidget(new QLabel("created by Kadir Arslan Ünal  "));
    statusBar()->setStyleSheet("color:#4a4b4f; font-size:10px;");
    statusBar()->showMessage("Scroll to zoom  ·  Middle-click to pan  ·  Click \"+ New Path\" to draw");
}

// ── project lifecycle ──────────────────────────────────────────────────────

void MainWindow::newProject() {
    if (m_isModified) {
        auto reply = QMessageBox::question(
            this, "Unsaved Changes",
            "The project has unsaved changes. Create new project anyway?",
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::No) return;
    }
    m_undoStack.clear();
    m_redoStack.clear();
    m_isModified = false;
    m_filePath.clear();
    delete m_project;
    m_project = new Project;
    refreshAllPanels();
    setWindowTitle("VEX V5 Path Planner — Untitled Project");
    statusBar()->showMessage("New project created.");
}

void MainWindow::openProject() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project", {},
        "VEX Path Planner (*.vpp);;JSON (*.json);;All Files (*)"
    );
    if (path.isEmpty()) return;

    auto* p = new Project;
    if (!ProjectSerializer::loadFromFile(path, *p)) {
        delete p;
        statusBar()->showMessage("Failed to open: " + path);
        return;
    }

    m_undoStack.clear();
    m_redoStack.clear();
    delete m_project;
    m_project    = p;
    m_filePath   = path;
    m_isModified = false;
    refreshAllPanels();
    setWindowTitle("VEX V5 Path Planner — " + QFileInfo(path).fileName());
    statusBar()->showMessage("Project opened.");
}

void MainWindow::saveProject() {
    if (m_filePath.isEmpty()) { saveProjectAs(); return; }
    if (ProjectSerializer::saveToFile(*m_project, m_filePath)) {
        m_project->markSaved();
        m_isModified = false;
        statusBar()->showMessage("Saved.");
    } else {
        statusBar()->showMessage("Save failed.");
    }
}

void MainWindow::saveProjectAs() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project As", {},
        "VEX Path Planner (*.vpp);;JSON (*.json)"
    );
    if (path.isEmpty()) return;
    if (!path.endsWith(".vpp", Qt::CaseInsensitive) &&
        !path.endsWith(".json", Qt::CaseInsensitive))
        path += ".vpp";
    m_filePath = path;
    saveProject();
    setWindowTitle("VEX V5 Path Planner — " + QFileInfo(path).fileName());
}

void MainWindow::refreshAllPanels() {
    m_scene->setProject(m_project);
    m_robotPanel->setProject(m_project);
    m_pathListPanel->setProject(m_project);
    m_pathListPanel->rebuild();
    m_outputPanel->setProject(m_project);
}

// ── undo / redo ────────────────────────────────────────────────────────────

void MainWindow::pushUndoState() {
    m_undoStack.append(ProjectSerializer::toJson(*m_project));
    if (m_undoStack.size() > 50)
        m_undoStack.removeFirst();
    m_redoStack.clear();
    m_isModified = true;
}

void MainWindow::undo() {
    if (m_undoStack.isEmpty()) {
        statusBar()->showMessage("Nothing to undo.");
        return;
    }
    m_redoStack.append(ProjectSerializer::toJson(*m_project));
    applyState(m_undoStack.takeLast());
    statusBar()->showMessage("Undo.");
}

void MainWindow::redo() {
    if (m_redoStack.isEmpty()) {
        statusBar()->showMessage("Nothing to redo.");
        return;
    }
    m_undoStack.append(ProjectSerializer::toJson(*m_project));
    applyState(m_redoStack.takeLast());
    statusBar()->showMessage("Redo.");
}

void MainWindow::applyState(const QByteArray& json) {
    auto* p = new Project;
    if (!ProjectSerializer::fromJson(json, *p)) { delete p; return; }
    delete m_project;
    m_project = p;
    refreshAllPanels();
}

// ── theme ──────────────────────────────────────────────────────────────────

void MainWindow::applyTheme(bool dark) {
    m_isDark = dark;
    QPalette pal;
    if (dark) {
        pal.setColor(QPalette::Window,          QColor(30, 31, 34));
        pal.setColor(QPalette::WindowText,      QColor(210, 210, 215));
        pal.setColor(QPalette::Base,            QColor(20, 21, 24));
        pal.setColor(QPalette::AlternateBase,   QColor(38, 39, 43));
        pal.setColor(QPalette::ToolTipBase,     QColor(25, 26, 30));
        pal.setColor(QPalette::ToolTipText,     QColor(210, 210, 215));
        pal.setColor(QPalette::Text,            QColor(210, 210, 215));
        pal.setColor(QPalette::Button,          QColor(45, 47, 52));
        pal.setColor(QPalette::ButtonText,      QColor(210, 210, 215));
        pal.setColor(QPalette::BrightText,      Qt::red);
        pal.setColor(QPalette::Highlight,       QColor(66, 133, 244));
        pal.setColor(QPalette::HighlightedText, Qt::white);
        pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(85, 86, 90));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(85, 86, 90));
    } else {
        pal = qApp->style()->standardPalette();
    }
    qApp->setPalette(pal);

    // Update panel backgrounds to match the theme
    m_robotPanel->refreshTheme(dark);
    m_pathListPanel->refreshTheme(dark);
    m_outputPanel->refreshTheme(dark);
}

// ── slots ──────────────────────────────────────────────────────────────────

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
