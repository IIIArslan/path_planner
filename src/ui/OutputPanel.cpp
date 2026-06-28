#include "OutputPanel.h"
#include "io/PythonExporter.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFont>
#include <QClipboard>
#include <QApplication>
#include <QTimer>

// ── style constants ────────────────────────────────────────────────────────

static const char* TEXT_AREA_SS =
    "QTextEdit {"
    "  background:#0f1012; color:#a8d8a8;"
    "  border:1px solid #2a2b30; border-radius:4px;"
    "  font-family:monospace; font-size:11px; padding:6px;"
    "}";

static const char* COPY_BTN_SS =
    "QPushButton {"
    "  background:#242528; color:#8a8b90;"
    "  border:1px solid #35363b; border-radius:4px;"
    "  padding:3px 10px; font-size:11px;"
    "}"
    "QPushButton:hover  { background:#2e2f34; color:#c8c9cd; }"
    "QPushButton:pressed { background:#3a3b40; }";

static const char* GEN_BTN_SS =
    "QPushButton {"
    "  background:#1a3a5a; color:#6ab0e8;"
    "  border:1px solid #2a5a8a; border-radius:4px;"
    "  padding:4px 14px; font-size:12px; font-weight:600;"
    "}"
    "QPushButton:hover  { background:#204878; color:#90c8f0; }"
    "QPushButton:pressed { background:#2a5890; }";

// ── construction ───────────────────────────────────────────────────────────

OutputPanel::OutputPanel(Project* project, FieldScene* scene, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
    , m_scene(scene)
{
    setStyleSheet("background:#18191c; border-top:1px solid #2a2b30;");
    setMinimumHeight(180);
    setMaximumHeight(300);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(6);

    // ── Control row ─────────────────────────────────────────────────────────
    auto* ctrlRow = new QHBoxLayout;
    ctrlRow->setSpacing(8);

    auto* pathLbl = new QLabel("Path:", this);
    pathLbl->setStyleSheet("color:#6a6b70; font-size:11px;");
    ctrlRow->addWidget(pathLbl);

    m_pathCombo = new QComboBox(this);
    m_pathCombo->setStyleSheet(
        "QComboBox { background:#242528; color:#c0c1c6; border:1px solid #35363b;"
        "  border-radius:4px; padding:3px 8px; font-size:11px; min-width:120px; }"
        "QComboBox::drop-down { border:none; }"
        "QComboBox QAbstractItemView { background:#242528; color:#c0c1c6; selection-background-color:#35363b; }"
    );
    ctrlRow->addWidget(m_pathCombo);

    auto* stepLbl = new QLabel("Step:", this);
    stepLbl->setStyleSheet("color:#6a6b70; font-size:11px;");
    ctrlRow->addWidget(stepLbl);

    m_stepSpin = new QDoubleSpinBox(this);
    m_stepSpin->setRange(0.1, 30.0);
    m_stepSpin->setValue(m_project->stepSizeCm);
    m_stepSpin->setSuffix(" cm");
    m_stepSpin->setDecimals(1);
    m_stepSpin->setFixedWidth(90);
    m_stepSpin->setStyleSheet(
        "QDoubleSpinBox { background:#242528; color:#c0c1c6; border:1px solid #35363b;"
        "  border-radius:4px; padding:3px 6px; font-size:11px; }"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:16px; }"
    );
    ctrlRow->addWidget(m_stepSpin);

    auto* genBtn = new QPushButton("▶  Generate", this);
    genBtn->setStyleSheet(GEN_BTN_SS);
    connect(genBtn, &QPushButton::clicked, this, &OutputPanel::generate);
    ctrlRow->addWidget(genBtn);

    m_statusLbl = new QLabel("", this);
    m_statusLbl->setStyleSheet("color:#50515a; font-size:10px; padding-left:8px;");
    ctrlRow->addWidget(m_statusLbl, 1);

    root->addLayout(ctrlRow);

    // ── Output areas ────────────────────────────────────────────────────────
    auto* areasRow = new QHBoxLayout;
    areasRow->setSpacing(8);

    auto buildColumn = [&](const QString& title, QTextEdit*& textRef) {
        auto* col = new QVBoxLayout;
        col->setSpacing(4);

        auto* header = new QHBoxLayout;
        auto* lbl    = new QLabel(title, this);
        lbl->setStyleSheet("color:#50515a; font-size:10px; font-weight:600; letter-spacing:0.5px;");
        header->addWidget(lbl);
        header->addStretch();

        textRef = makeTextArea(this);
        auto* copyBtn = makeCopyBtn(textRef, this);
        header->addWidget(copyBtn);

        col->addLayout(header);
        col->addWidget(textRef, 1);
        areasRow->addLayout(col, 1);
    };

    buildColumn("WAYPOINTS", m_wpText);
    buildColumn("HEADINGS",  m_hdText);

    root->addLayout(areasRow, 1);

    // Keep project step size in sync
    connect(m_stepSpin, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        m_project->stepSizeCm = v;
    });

    // Sync combo when paths change
    connect(m_scene, &FieldScene::pathCountChanged,    this, [this](int) { refreshPathList(); });
    connect(m_scene, &FieldScene::pathSelectionChanged, this, [this](int idx) {
        if (idx >= 0 && idx < m_pathCombo->count())
            m_pathCombo->setCurrentIndex(idx);
    });

    refreshPathList();
}

// ── public ─────────────────────────────────────────────────────────────────

void OutputPanel::setProject(Project* p) {
    m_project = p;
    QSignalBlocker b(m_stepSpin);
    m_stepSpin->setValue(p->stepSizeCm);
    refreshPathList();
}

void OutputPanel::refreshPathList() {
    m_pathCombo->blockSignals(true);
    m_pathCombo->clear();
    for (const auto& path : m_project->paths)
        m_pathCombo->addItem(QString::fromStdString(path.name));
    m_pathCombo->blockSignals(false);
}

// ── private ────────────────────────────────────────────────────────────────

void OutputPanel::generate() {
    int idx = m_pathCombo->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(m_project->paths.size())) {
        m_wpText->setPlainText("# No path selected.");
        m_hdText->setPlainText("# No path selected.");
        m_statusLbl->setText("");
        return;
    }

    double stepCm = m_stepSpin->value();
    auto   out    = PythonExporter::generate(m_project->paths[idx], stepCm);

    m_wpText->setPlainText(out.waypointCode);
    m_hdText->setPlainText(out.headingCode);
    m_statusLbl->setText(
        QString("Generated %1 waypoints  ·  step = %2 cm")
            .arg(out.pointCount)
            .arg(stepCm, 0, 'f', 1)
    );
}

QTextEdit* OutputPanel::makeTextArea(QWidget* parent) {
    auto* te = new QTextEdit(parent);
    te->setReadOnly(true);
    te->setStyleSheet(TEXT_AREA_SS);
    QFont f("Monospace");
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(10);
    te->setFont(f);
    te->setPlaceholderText("Click ▶ Generate to produce output.");
    return te;
}

QPushButton* OutputPanel::makeCopyBtn(QTextEdit* target, QWidget* parent) {
    auto* btn = new QPushButton("⎘ Copy", parent);
    btn->setStyleSheet(COPY_BTN_SS);
    QObject::connect(btn, &QPushButton::clicked, [target, btn]() {
        QApplication::clipboard()->setText(target->toPlainText());
        btn->setText("✓ Copied");
        QTimer::singleShot(1500, btn, [btn]() { btn->setText("⎘ Copy"); });
    });
    return btn;
}
