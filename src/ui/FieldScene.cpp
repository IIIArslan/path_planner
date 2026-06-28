#include "FieldScene.h"
#include "core/FieldConstants.h"
#include <QPen>
#include <QBrush>
#include <QColor>

FieldScene::FieldScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-Field::HALF_CM, -Field::HALF_CM, Field::SIZE_CM, Field::SIZE_CM);
    buildBackground();
    buildGrid();
}

void FieldScene::setProject(Project* project) {
    m_project = project;
}

void FieldScene::loadFieldImage(const QString& path) {
    QPixmap px(path);
    if (px.isNull()) return;

    if (!m_img) {
        m_img = addPixmap(px);
        m_img->setZValue(-1);
    } else {
        m_img->setPixmap(px);
    }

    // Scale image to fill the field area exactly
    double sx = Field::SIZE_CM / px.width();
    double sy = Field::SIZE_CM / px.height();
    m_img->setPos(-Field::HALF_CM, -Field::HALF_CM);
    m_img->setTransform(QTransform::fromScale(sx, sy));

    if (m_bg) m_bg->setVisible(false);
}

void FieldScene::clearFieldImage() {
    if (m_img) {
        removeItem(m_img);
        delete m_img;
        m_img = nullptr;
    }
    if (m_bg) m_bg->setVisible(true);
}

void FieldScene::buildBackground() {
    // Dark green carpet — replaced by image when one is loaded
    m_bg = addRect(
        -Field::HALF_CM, -Field::HALF_CM, Field::SIZE_CM, Field::SIZE_CM,
        QPen(Qt::NoPen),
        QBrush(QColor(28, 72, 42))
    );
    m_bg->setZValue(-2);
}

void FieldScene::buildGrid() {
    for (auto* item : m_grid) { removeItem(item); delete item; }
    m_grid.clear();

    // Interior dividers: cosmetic dotted white, semi-transparent
    QPen dotPen(QColor(255, 255, 255, 80));
    dotPen.setWidthF(1.0);
    dotPen.setCosmetic(true);
    dotPen.setStyle(Qt::DotLine);

    // Field border: solid, fully opaque
    QPen edgePen(QColor(255, 255, 255, 210));
    edgePen.setWidthF(2.0);
    edgePen.setCosmetic(true);

    const double h = Field::HALF_CM;
    const double t = Field::TILE_CM;

    for (int i = 0; i <= Field::TILES; ++i) {
        double pos    = -h + i * t;
        bool   isEdge = (i == 0 || i == Field::TILES);
        const QPen& pen = isEdge ? edgePen : dotPen;

        auto* vl = addLine(pos, -h, pos,  h, pen);
        auto* hl = addLine(-h, pos,  h, pos, pen);
        vl->setZValue(1);
        hl->setZValue(1);
        m_grid << vl << hl;
    }
}
