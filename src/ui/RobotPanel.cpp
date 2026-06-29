#include "RobotPanel.h"
#include "core/FieldConstants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <cmath>

static const char* SPIN_SS =
    "QDoubleSpinBox {"
    "  background:#242528; color:#c0c1c6;"
    "  border:1px solid #35363b; border-radius:4px;"
    "  padding:2px 4px; font-size:11px;"
    "}"
    "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:14px; }";

static QDoubleSpinBox* makeSpin(double lo, double hi, double val,
                                 const QString& sfx, QWidget* p, int dec = 1)
{
    auto* s = new QDoubleSpinBox(p);
    s->setRange(lo, hi);
    s->setValue(val);
    s->setSuffix(sfx);
    s->setDecimals(dec);
    s->setStyleSheet(SPIN_SS);
    return s;
}

static QLabel* lbl(const QString& t, QWidget* p) {
    auto* l = new QLabel(t, p);
    l->setStyleSheet("color:#6a6b70; font-size:11px;");
    return l;
}

RobotPanel::RobotPanel(Project* project, FieldScene* scene, QWidget* parent)
    : QWidget(parent), m_project(project), m_scene(scene)
{
    setFixedWidth(220);
    setStyleSheet("background:#1a1b1e;");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(6);

    // ── Tab toggle row ───────────────────────────────────────────────────────
    auto* tabRow = new QHBoxLayout;
    tabRow->setSpacing(4);
    m_robotTabBtn = new QPushButton("Robot", this);
    m_velTabBtn   = new QPushButton("Velocity", this);
    tabRow->addWidget(m_robotTabBtn);
    tabRow->addWidget(m_velTabBtn);
    root->addLayout(tabRow);

    // ── Stacked pages ────────────────────────────────────────────────────────
    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack);
    root->addStretch();

    // ── Page 0: Robot config ─────────────────────────────────────────────────
    {
        auto* page = new QWidget;
        auto* pl   = new QVBoxLayout(page);
        pl->setContentsMargins(0, 4, 0, 0);
        pl->setSpacing(6);

        auto* hdr   = new QHBoxLayout;
        auto* title = new QLabel("ROBOT", page);
        title->setStyleSheet("color:#5a5b60; font-weight:700; font-size:10px; letter-spacing:1px;");
        hdr->addWidget(title);
        hdr->addStretch();
        m_visCheck = new QCheckBox("Show", page);
        m_visCheck->setChecked(true);
        m_visCheck->setStyleSheet(
            "QCheckBox { color:#6a6b70; font-size:11px; }"
            "QCheckBox::indicator { width:13px; height:13px; }"
        );
        hdr->addWidget(m_visCheck);
        pl->addLayout(hdr);

        auto* xyRow = new QHBoxLayout;
        xyRow->setSpacing(5);
        xyRow->addWidget(lbl("X:", page));
        m_xSpin = makeSpin(-Field::HALF_CM, Field::HALF_CM, m_project->robotConfig.startX, " cm", page);
        m_xSpin->setFixedWidth(82);
        xyRow->addWidget(m_xSpin);
        xyRow->addWidget(lbl("Y:", page));
        m_ySpin = makeSpin(-Field::HALF_CM, Field::HALF_CM, m_project->robotConfig.startY, " cm", page);
        m_ySpin->setFixedWidth(82);
        xyRow->addWidget(m_ySpin);
        pl->addLayout(xyRow);

        auto* hdgRow = new QHBoxLayout;
        hdgRow->setSpacing(5);
        hdgRow->addWidget(lbl("Heading:", page));
        m_hdgSpin = makeSpin(0.0, 359.9, m_project->robotConfig.startHeading, " °", page);
        m_hdgSpin->setWrapping(true);
        m_hdgSpin->setFixedWidth(90);
        hdgRow->addWidget(m_hdgSpin);
        hdgRow->addStretch();
        pl->addLayout(hdgRow);

        auto* szRow = new QHBoxLayout;
        szRow->setSpacing(5);
        szRow->addWidget(lbl("W:", page));
        m_wSpin = makeSpin(10.0, 100.0, m_project->robotConfig.widthCm, " cm", page);
        m_wSpin->setFixedWidth(74);
        szRow->addWidget(m_wSpin);
        szRow->addWidget(lbl("H:", page));
        m_hSpin = makeSpin(10.0, 100.0, m_project->robotConfig.heightCm, " cm", page);
        m_hSpin->setFixedWidth(74);
        szRow->addWidget(m_hSpin);
        pl->addLayout(szRow);

        pl->addStretch();
        m_stack->addWidget(page);  // index 0
    }

    // ── Page 1: Velocity profile ──────────────────────────────────────────────
    {
        auto* page = new QWidget;
        auto* pl   = new QVBoxLayout(page);
        pl->setContentsMargins(0, 4, 0, 0);
        pl->setSpacing(6);

        auto* velHdr = new QLabel("VELOCITY PROFILE", page);
        velHdr->setStyleSheet("color:#5a5b60; font-weight:700; font-size:10px; letter-spacing:1px;");
        pl->addWidget(velHdr);

        auto* vRow1 = new QHBoxLayout;
        vRow1->setSpacing(5);
        vRow1->addWidget(lbl("k:", page));
        m_kCurveSpin = makeSpin(0.1, 200.0, m_project->robotConfig.kCurve, "", page, 1);
        m_kCurveSpin->setFixedWidth(66);
        vRow1->addWidget(m_kCurveSpin);
        vRow1->addWidget(lbl("v_min:", page));
        m_vMinSpin = makeSpin(0.0, 0.99, m_project->robotConfig.vMin, "", page, 2);
        m_vMinSpin->setFixedWidth(58);
        vRow1->addWidget(m_vMinSpin);
        vRow1->addStretch();
        pl->addLayout(vRow1);

        auto* vRow2 = new QHBoxLayout;
        vRow2->setSpacing(5);
        vRow2->addWidget(lbl("accel:", page));
        m_aMaxSpin = makeSpin(0.001, 0.05, m_project->robotConfig.aMax, "", page, 3);
        m_aMaxSpin->setFixedWidth(62);
        vRow2->addWidget(m_aMaxSpin);
        vRow2->addWidget(lbl("ahead:", page));
        m_lookAheadSpin = makeSpin(1.0, 200.0, m_project->robotConfig.lookAheadCm, " cm", page, 0);
        m_lookAheadSpin->setFixedWidth(66);
        vRow2->addWidget(m_lookAheadSpin);
        vRow2->addStretch();
        pl->addLayout(vRow2);

        auto* vRow3 = new QHBoxLayout;
        vRow3->setSpacing(5);
        vRow3->addWidget(lbl("v_end:", page));
        m_vEndSpin = makeSpin(0.0, 1.0, m_project->robotConfig.vEnd, "", page, 2);
        m_vEndSpin->setFixedWidth(66);
        vRow3->addWidget(m_vEndSpin);
        vRow3->addStretch();
        pl->addLayout(vRow3);

        pl->addStretch();
        m_stack->addWidget(page);  // index 1
    }

    // ── Connections ───────────────────────────────────────────────────────────
    auto push = [this](double) { pushToScene(); };
    connect(m_xSpin,         &QDoubleSpinBox::valueChanged, this, push);
    connect(m_ySpin,         &QDoubleSpinBox::valueChanged, this, push);
    connect(m_hdgSpin,       &QDoubleSpinBox::valueChanged, this, push);
    connect(m_wSpin,         &QDoubleSpinBox::valueChanged, this, push);
    connect(m_hSpin,         &QDoubleSpinBox::valueChanged, this, push);
    connect(m_kCurveSpin,    &QDoubleSpinBox::valueChanged, this, push);
    connect(m_vMinSpin,      &QDoubleSpinBox::valueChanged, this, push);
    connect(m_aMaxSpin,      &QDoubleSpinBox::valueChanged, this, push);
    connect(m_lookAheadSpin, &QDoubleSpinBox::valueChanged, this, push);
    connect(m_vEndSpin,      &QDoubleSpinBox::valueChanged, this, push);

    connect(m_visCheck,    &QCheckBox::toggled,     m_scene, &FieldScene::setRobotVisible);
    connect(m_scene,       &FieldScene::robotMoved, this,    &RobotPanel::onRobotMoved);
    connect(m_robotTabBtn, &QPushButton::clicked,   [this]() { setTab(0); });
    connect(m_velTabBtn,   &QPushButton::clicked,   [this]() { setTab(1); });

    setTab(0);
}

void RobotPanel::refreshTheme(bool dark) {
    m_isDark = dark;
    setStyleSheet(dark ? "background:#1a1b1e;" : "background:#f0f1f4;");

    const char* spinSS = dark
        ? "QDoubleSpinBox {"
          "  background:#242528; color:#c0c1c6;"
          "  border:1px solid #35363b; border-radius:4px;"
          "  padding:2px 4px; font-size:11px;"
          "}"
          "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:14px; }"
        : "QDoubleSpinBox {"
          "  background:#ffffff; color:#1a1b1e;"
          "  border:1px solid #c0c1c5; border-radius:4px;"
          "  padding:2px 4px; font-size:11px;"
          "}"
          "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:14px; }";
    for (auto* s : findChildren<QDoubleSpinBox*>())
        s->setStyleSheet(spinSS);

    for (auto* l : findChildren<QLabel*>()) {
        const QString txt = l->text();
        if (txt == "ROBOT" || txt == "VELOCITY PROFILE")
            l->setStyleSheet(dark
                ? "color:#5a5b60; font-weight:700; font-size:10px; letter-spacing:1px;"
                : "color:#8a8b90; font-weight:700; font-size:10px; letter-spacing:1px;");
        else
            l->setStyleSheet(dark
                ? "color:#6a6b70; font-size:11px;"
                : "color:#4a4b50; font-size:11px;");
    }

    m_visCheck->setStyleSheet(dark
        ? "QCheckBox { color:#6a6b70; font-size:11px; }"
          "QCheckBox::indicator { width:13px; height:13px; }"
        : "QCheckBox { color:#4a4b50; font-size:11px; }"
          "QCheckBox::indicator { width:13px; height:13px; }");

    setTab(m_stack->currentIndex());
}

void RobotPanel::setProject(Project* p) {
    m_project = p;
    QSignalBlocker bx(m_xSpin), by(m_ySpin), bh(m_hdgSpin), bw(m_wSpin), bhh(m_hSpin);
    QSignalBlocker bk(m_kCurveSpin), bvm(m_vMinSpin), bam(m_aMaxSpin),
                   bla(m_lookAheadSpin), bve(m_vEndSpin);
    m_xSpin->setValue(p->robotConfig.startX);
    m_ySpin->setValue(p->robotConfig.startY);
    m_hdgSpin->setValue(p->robotConfig.startHeading);
    m_wSpin->setValue(p->robotConfig.widthCm);
    m_hSpin->setValue(p->robotConfig.heightCm);
    m_kCurveSpin->setValue(p->robotConfig.kCurve);
    m_vMinSpin->setValue(p->robotConfig.vMin);
    m_aMaxSpin->setValue(p->robotConfig.aMax);
    m_lookAheadSpin->setValue(p->robotConfig.lookAheadCm);
    m_vEndSpin->setValue(p->robotConfig.vEnd);
    m_scene->applyRobotConfig(p->robotConfig);
}

void RobotPanel::onRobotMoved(double x, double y, double heading) {
    double h = std::fmod(heading, 360.0);
    if (h < 0.0) h += 360.0;
    QSignalBlocker bx(m_xSpin), by(m_ySpin), bh(m_hdgSpin);
    m_xSpin->setValue(x);
    m_ySpin->setValue(y);
    m_hdgSpin->setValue(h);
}

void RobotPanel::pushToScene() {
    RobotConfig cfg;
    cfg.startX       = m_xSpin->value();
    cfg.startY       = m_ySpin->value();
    cfg.startHeading = m_hdgSpin->value();
    cfg.widthCm      = m_wSpin->value();
    cfg.heightCm     = m_hSpin->value();
    cfg.kCurve       = m_kCurveSpin->value();
    cfg.vMin         = m_vMinSpin->value();
    cfg.aMax         = m_aMaxSpin->value();
    cfg.lookAheadCm  = m_lookAheadSpin->value();
    cfg.vEnd         = m_vEndSpin->value();
    m_project->robotConfig = cfg;
    m_scene->applyRobotConfig(cfg);
}

void RobotPanel::setTab(int idx) {
    m_stack->setCurrentIndex(idx);

    QString activeSS = m_isDark
        ? "QPushButton { background:#1a3a5a; color:#6ab0e8;"
          " border:1px solid #2a5a8a; border-radius:4px;"
          " padding:3px 0; font-size:11px; font-weight:600; }"
        : "QPushButton { background:#1a6abf; color:#ffffff;"
          " border:1px solid #1055a0; border-radius:4px;"
          " padding:3px 0; font-size:11px; font-weight:600; }";

    QString inactiveSS = m_isDark
        ? "QPushButton { background:#242528; color:#6a6b70;"
          " border:1px solid #35363b; border-radius:4px;"
          " padding:3px 0; font-size:11px; }"
          "QPushButton:hover { background:#2e2f34; color:#a0a1a6; }"
        : "QPushButton { background:#e2e3e7; color:#5a5b60;"
          " border:1px solid #c0c1c5; border-radius:4px;"
          " padding:3px 0; font-size:11px; }"
          "QPushButton:hover { background:#d4d5d9; }";

    m_robotTabBtn->setStyleSheet(idx == 0 ? activeSS : inactiveSS);
    m_velTabBtn->setStyleSheet(idx == 1 ? activeSS : inactiveSS);
}
