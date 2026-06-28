#include "PathListPanel.h"
#include "core/PathInterpolator.h"
#include "core/FieldConstants.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QColorDialog>
#include <QMenu>
#include <QEvent>
#include <QMouseEvent>

// ── style helpers ──────────────────────────────────────────────────────────

static constexpr int ROW_H = 44;

static QString colorDotStyle(int r, int g, int b) {
    return QString(
        "QPushButton {"
        "  background:rgb(%1,%2,%3);"
        "  border-radius:7px; border:2px solid rgba(255,255,255,35);"
        "  min-width:14px; max-width:14px; min-height:14px; max-height:14px;"
        "}"
        "QPushButton:hover { border:2px solid rgba(255,255,255,100); }"
    ).arg(r).arg(g).arg(b);
}

static const char* ICON_BTN_STYLE =
    "QPushButton {"
    "  background:transparent; color:#707278;"
    "  border:none; font-size:14px; padding:0 4px;"
    "}"
    "QPushButton:hover { color:#c8c9cd; }";

static const char* DELETE_BTN_STYLE =
    "QPushButton {"
    "  background:transparent; color:#505258;"
    "  border:none; font-size:14px; padding:0 4px;"
    "}"
    "QPushButton:hover { color:#e05555; }";

static const char* NAME_EDIT_STYLE =
    "QLineEdit {"
    "  background:transparent; border:none;"
    "  color:#c8c9cd; font-size:12px; padding:0 2px;"
    "}"
    "QLineEdit:focus {"
    "  background:#242528; border:1px solid #44454a; border-radius:3px;"
    "  padding:0 4px;"
    "}";

// ── construction ───────────────────────────────────────────────────────────

PathListPanel::PathListPanel(Project* project, FieldScene* scene, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
    , m_scene(scene)
{
    setFixedWidth(220);
    setStyleSheet("background:#1a1b1e;");

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ── Header ─────────────────────────────────────────────────────────────
    auto* header = new QLabel("PATHS", this);
    header->setContentsMargins(14, 14, 14, 8);
    header->setStyleSheet("color:#5a5b60; font-weight:700; font-size:10px; letter-spacing:1px;");
    outerLayout->addWidget(header);

    // ── "+" button ─────────────────────────────────────────────────────────
    auto* addBtn = new QPushButton("+ New Path", this);
    addBtn->setStyleSheet(
        "QPushButton {"
        "  background:#242528; color:#a0a1a6;"
        "  border:1px solid #35363b; border-radius:5px;"
        "  padding:7px 12px; margin:0 10px 10px 10px; font-size:12px;"
        "}"
        "QPushButton:hover { background:#2e2f34; color:#d0d1d6; }"
        "QPushButton:pressed { background:#3a3b40; }"
    );
    connect(addBtn, &QPushButton::clicked, m_scene, &FieldScene::startNewPath);
    outerLayout->addWidget(addBtn);

    // ── Separator ──────────────────────────────────────────────────────────
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#252630;");
    outerLayout->addWidget(sep);

    // ── Scrollable row list ────────────────────────────────────────────────
    auto* scrollContainer = new QWidget(this);
    m_rowLayout = new QVBoxLayout(scrollContainer);
    m_rowLayout->setContentsMargins(0, 4, 0, 4);
    m_rowLayout->setSpacing(0);
    m_rowLayout->addStretch();

    auto* scroll = new QScrollArea(this);
    scroll->setWidget(scrollContainer);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("background:transparent;");
    outerLayout->addWidget(scroll, 1);

    // ── Info bar at the bottom ──────────────────────────────────────────────
    auto* infoBar = new QWidget(this);
    infoBar->setStyleSheet("background:#14151a; border-top:1px solid #252630;");
    auto* infoLayout = new QVBoxLayout(infoBar);
    infoLayout->setContentsMargins(10, 6, 10, 6);
    infoLayout->setSpacing(4);

    m_infoLabel = new QLabel("No path selected", infoBar);
    m_infoLabel->setStyleSheet("color:#484850; font-size:10px;");
    m_infoLabel->setWordWrap(true);
    infoLayout->addWidget(m_infoLabel);

    // Position editors (shown only when a non-empty path is selected)
    m_posWidget = new QWidget(infoBar);
    m_posWidget->setVisible(false);
    auto* posLayout = new QVBoxLayout(m_posWidget);
    posLayout->setContentsMargins(0, 2, 0, 0);
    posLayout->setSpacing(3);

    static const char* POS_SPIN_SS =
        "QDoubleSpinBox {"
        "  background:#242528; color:#c0c1c6;"
        "  border:1px solid #35363b; border-radius:3px;"
        "  padding:1px 2px; font-size:10px;"
        "}"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:12px; }";

    auto makePosRow = [&](const QString& rowLabel,
                          QDoubleSpinBox*& xSpin, QDoubleSpinBox*& ySpin) {
        auto* row = new QHBoxLayout;
        row->setSpacing(3);

        auto* lbl = new QLabel(rowLabel, m_posWidget);
        lbl->setStyleSheet("color:#5a5b60; font-size:10px; min-width:26px;");
        row->addWidget(lbl);

        auto makeXY = [&](const QString& axis, QDoubleSpinBox*& spin) {
            auto* axLbl = new QLabel(axis, m_posWidget);
            axLbl->setStyleSheet("color:#5a5b60; font-size:10px;");
            row->addWidget(axLbl);
            spin = new QDoubleSpinBox(m_posWidget);
            spin->setRange(-Field::HALF_CM, Field::HALF_CM);
            spin->setDecimals(1);
            spin->setFixedWidth(58);
            spin->setStyleSheet(POS_SPIN_SS);
            row->addWidget(spin);
        };
        makeXY("X", xSpin);
        makeXY("Y", ySpin);
        posLayout->addLayout(row);
    };

    makePosRow("Start", m_startXSpin, m_startYSpin);
    makePosRow("End",   m_endXSpin,   m_endYSpin);
    infoLayout->addWidget(m_posWidget);
    outerLayout->addWidget(infoBar);

    connect(m_startXSpin, &QAbstractSpinBox::editingFinished, this, &PathListPanel::onStartPosChanged);
    connect(m_startYSpin, &QAbstractSpinBox::editingFinished, this, &PathListPanel::onStartPosChanged);
    connect(m_endXSpin,   &QAbstractSpinBox::editingFinished, this, &PathListPanel::onEndPosChanged);
    connect(m_endYSpin,   &QAbstractSpinBox::editingFinished, this, &PathListPanel::onEndPosChanged);

    // Connect scene signals
    connect(m_scene, &FieldScene::pathCountChanged, this, [this](int) { rebuild(); });
    connect(m_scene, &FieldScene::pathSelectionChanged, this, [this](int idx) {
        setSelectedRow(idx);
        updateInfo(idx);
    });
    connect(m_scene, &FieldScene::pathCountChanged, this, [this](int) {
        updateInfo(m_selectedIdx);
    });
}

// ── public API ─────────────────────────────────────────────────────────────

void PathListPanel::refreshTheme(bool dark) {
    setStyleSheet(dark ? "background:#1a1b1e;" : "");
    // Info bar background
    if (auto* bar = m_infoLabel ? m_infoLabel->parentWidget() : nullptr)
        bar->setStyleSheet(dark ? "background:#14151a; border-top:1px solid #252630;" : "");
}

void PathListPanel::rebuild() {
    clearRows();
    for (int i = 0; i < static_cast<int>(m_project->paths.size()); ++i)
        addRow(i);
    setSelectedRow(m_scene->selectedPathIdx());
}

void PathListPanel::setSelectedRow(int idx) {
    m_selectedIdx = idx;
    for (int i = 0; i < m_rows.size(); ++i)
        applyRowStyle(m_rows[i], i == idx);
}

// ── event filter (row click → select path) ────────────────────────────────

bool PathListPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto* row = qobject_cast<QFrame*>(watched);
        if (row) {
            int idx = m_rows.indexOf(row);
            if (idx >= 0)
                m_scene->selectPathAt(idx);
        }
    }
    return false;
}

// ── private ────────────────────────────────────────────────────────────────

void PathListPanel::addRow(int pathIdx) {
    Path& path = m_project->paths[pathIdx];

    auto* row = new QFrame;
    row->setFixedHeight(ROW_H);
    row->setFrameShape(QFrame::NoFrame);
    row->installEventFilter(this);
    applyRowStyle(row, false);

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 0, 6, 0);
    layout->setSpacing(6);

    // ── Colour dot ──────────────────────────────────────────────────────────
    const auto& c = path.color;
    auto* colorBtn = new QPushButton(row);
    colorBtn->setStyleSheet(colorDotStyle(c.r, c.g, c.b));
    colorBtn->setFixedSize(14, 14);
    connect(colorBtn, &QPushButton::clicked, this, [this, pathIdx, colorBtn]() {
        m_scene->beginEdit();
        openColorPicker(pathIdx);
        // Refresh dot colour after picker closes
        const auto& nc = m_project->paths[pathIdx].color;
        colorBtn->setStyleSheet(colorDotStyle(nc.r, nc.g, nc.b));
    });
    layout->addWidget(colorBtn);

    // ── Name field ──────────────────────────────────────────────────────────
    auto* nameEdit = new QLineEdit(QString::fromStdString(path.name), row);
    nameEdit->setStyleSheet(NAME_EDIT_STYLE);
    nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(nameEdit, &QLineEdit::editingFinished, this, [this, pathIdx, nameEdit]() {
        if (pathIdx < static_cast<int>(m_project->paths.size())) {
            m_scene->beginEdit();
            m_project->paths[pathIdx].name = nameEdit->text().toStdString();
        }
    });
    // Selecting the path when name field is clicked
    connect(nameEdit, &QLineEdit::selectionChanged, this, [this, pathIdx]() {
        m_scene->selectPathAt(pathIdx);
    });
    layout->addWidget(nameEdit);

    // ── Visibility toggle ───────────────────────────────────────────────────
    auto* eyeBtn = new QPushButton(path.visible ? "◉" : "◎", row);
    eyeBtn->setStyleSheet(ICON_BTN_STYLE);
    eyeBtn->setFixedSize(24, 24);
    eyeBtn->setToolTip("Toggle visibility");
    connect(eyeBtn, &QPushButton::clicked, this, [this, pathIdx, eyeBtn]() {
        if (pathIdx >= static_cast<int>(m_project->paths.size())) return;
        m_scene->beginEdit();
        Path& p  = m_project->paths[pathIdx];
        p.visible = !p.visible;
        eyeBtn->setText(p.visible ? "◉" : "◎");
        m_scene->refreshPathItem(pathIdx);
    });
    layout->addWidget(eyeBtn);

    // ── Merge button ────────────────────────────────────────────────────────
    auto* mergeBtn = new QPushButton("⊕", row);
    mergeBtn->setStyleSheet(ICON_BTN_STYLE);
    mergeBtn->setFixedSize(24, 24);
    mergeBtn->setToolTip("Merge with another path");
    connect(mergeBtn, &QPushButton::clicked, this, [this, pathIdx, mergeBtn]() {
        if (static_cast<int>(m_project->paths.size()) < 2) return;
        QMenu menu;
        for (int i = 0; i < static_cast<int>(m_project->paths.size()); ++i) {
            if (i == pathIdx) continue;
            menu.addAction(QString::fromStdString(m_project->paths[i].name),
                [this, pathIdx, i]() { m_scene->mergePathsAt(pathIdx, i); });
        }
        menu.exec(mergeBtn->mapToGlobal(mergeBtn->rect().bottomLeft()));
    });
    layout->addWidget(mergeBtn);

    // ── Delete button ───────────────────────────────────────────────────────
    auto* delBtn = new QPushButton("✕", row);
    delBtn->setStyleSheet(DELETE_BTN_STYLE);
    delBtn->setFixedSize(24, 24);
    delBtn->setToolTip("Delete path");
    connect(delBtn, &QPushButton::clicked, this, [this, pathIdx]() {
        m_scene->removePath(pathIdx);
        // rebuild() will be called via pathCountChanged signal
    });
    layout->addWidget(delBtn);

    // Insert before the trailing stretch
    m_rowLayout->insertWidget(m_rowLayout->count() - 1, row);
    m_rows.append(row);
}

void PathListPanel::clearRows() {
    for (auto* row : m_rows) {
        m_rowLayout->removeWidget(row);
        delete row;
    }
    m_rows.clear();
}

void PathListPanel::applyRowStyle(QFrame* row, bool selected) {
    if (selected) {
        row->setStyleSheet(
            "QFrame { background:#22232a; border-left:3px solid #4285f4; }"
        );
    } else {
        row->setStyleSheet(
            "QFrame { background:transparent; border-left:3px solid transparent; }"
            "QFrame:hover { background:#1f2025; }"
        );
    }
}

void PathListPanel::openColorPicker(int pathIdx) {
    if (pathIdx >= static_cast<int>(m_project->paths.size())) return;
    auto& c = m_project->paths[pathIdx].color;
    QColor initial(c.r, c.g, c.b);
    QColor chosen = QColorDialog::getColor(initial, this, "Choose Path Colour");
    if (!chosen.isValid()) return;
    c.r = static_cast<uint8_t>(chosen.red());
    c.g = static_cast<uint8_t>(chosen.green());
    c.b = static_cast<uint8_t>(chosen.blue());
    m_scene->refreshPathItem(pathIdx);
}

void PathListPanel::onStartPosChanged() {
    if (m_selectedIdx < 0 || m_selectedIdx >= static_cast<int>(m_project->paths.size())) return;
    Path& p = m_project->paths[m_selectedIdx];
    if (p.isEmpty()) return;
    double newX = m_startXSpin->value();
    double newY = m_startYSpin->value();
    Vec2& p0 = p.segments.front().p0;
    Vec2& p1 = p.segments.front().p1;
    double dx = newX - p0.x;
    double dy = newY - p0.y;
    m_scene->beginEdit();
    p0.x = newX; p0.y = newY;
    p1.x += dx;  p1.y += dy;
    m_project->markModified();
    m_scene->refreshPathItemFromModel(m_selectedIdx);
}

void PathListPanel::onEndPosChanged() {
    if (m_selectedIdx < 0 || m_selectedIdx >= static_cast<int>(m_project->paths.size())) return;
    Path& p = m_project->paths[m_selectedIdx];
    if (p.isEmpty()) return;
    double newX = m_endXSpin->value();
    double newY = m_endYSpin->value();
    Vec2& p3 = p.segments.back().p3;
    Vec2& p2 = p.segments.back().p2;
    double dx = newX - p3.x;
    double dy = newY - p3.y;
    m_scene->beginEdit();
    p3.x = newX; p3.y = newY;
    p2.x += dx;  p2.y += dy;
    m_project->markModified();
    m_scene->refreshPathItemFromModel(m_selectedIdx);
}

void PathListPanel::updateInfo(int idx) {
    if (idx < 0 || idx >= static_cast<int>(m_project->paths.size())) {
        m_infoLabel->setText("No path selected");
        m_posWidget->setVisible(false);
        return;
    }
    const Path& path = m_project->paths[idx];
    if (path.isEmpty()) {
        m_infoLabel->setText(QString::fromStdString(path.name) + " — empty path");
        m_posWidget->setVisible(false);
        return;
    }
    double len   = path.totalLength();
    double step  = m_project->stepSizeCm;
    int    count = static_cast<int>(PathInterpolator::interpolate(path, step).size());
    m_infoLabel->setText(
        QString("%1 cm  ·  ~%2 pts @ %3 cm step")
            .arg(len, 0, 'f', 1)
            .arg(count)
            .arg(step, 0, 'f', 1)
    );

    // Populate position spinboxes without triggering change handlers
    {
        QSignalBlocker bsx(*m_startXSpin), bsy(*m_startYSpin),
                       bex(*m_endXSpin),   bey(*m_endYSpin);
        m_startXSpin->setValue(path.segments.front().p0.x);
        m_startYSpin->setValue(path.segments.front().p0.y);
        m_endXSpin->setValue(path.segments.back().p3.x);
        m_endYSpin->setValue(path.segments.back().p3.y);
    }
    m_posWidget->setVisible(true);
}
