#include "FieldView.h"
#include "FieldScene.h"
#include "core/FieldConstants.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QShowEvent>
#include <QScrollBar>
#include <cmath>

FieldView::FieldView(QWidget* parent)
    : QGraphicsView(parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setBackgroundBrush(QColor(18, 18, 22));

    // Floating coordinate tooltip — rendered over the view, not inside the scene
    m_coordLabel = new QLabel(this);
    m_coordLabel->setStyleSheet(
        "background-color: rgba(0,0,0,172);"
        "color: #e8e8e8;"
        "padding: 3px 8px;"
        "border-radius: 4px;"
        "font-family: monospace;"
        "font-size: 11px;"
    );
    m_coordLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_coordLabel->hide();
}

void FieldView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    if (!m_fittedOnce && scene()) {
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
        m_fittedOnce = true;
    }
}

void FieldView::wheelEvent(QWheelEvent* event) {
    double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    scale(factor, factor);
}

void FieldView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_panning    = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void FieldView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::CrossCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void FieldView::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }
    updateCoordLabel(event->pos());
    QGraphicsView::mouseMoveEvent(event);
}

void FieldView::leaveEvent(QEvent* event) {
    m_coordLabel->hide();
    QGraphicsView::leaveEvent(event);
}

void FieldView::updateCoordLabel(const QPoint& viewPos) {
    QPointF field = FieldScene::sceneToField(mapToScene(viewPos));

    if (std::abs(field.x()) > Field::HALF_CM || std::abs(field.y()) > Field::HALF_CM) {
        m_coordLabel->hide();
        return;
    }

    m_coordLabel->setText(
        QString("(%1, %2) cm")
            .arg(field.x(), 0, 'f', 1)
            .arg(field.y(), 0, 'f', 1)
    );
    m_coordLabel->adjustSize();

    // Position to the right/below the cursor; mirror if too close to an edge
    QPoint pos = viewPos + QPoint(16, 10);
    if (pos.x() + m_coordLabel->width()  > width())  pos.rx() = viewPos.x() - m_coordLabel->width()  - 8;
    if (pos.y() + m_coordLabel->height() > height()) pos.ry() = viewPos.y() - m_coordLabel->height() - 8;
    m_coordLabel->move(pos);
    m_coordLabel->show();
}
