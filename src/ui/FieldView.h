#pragma once
#include <QGraphicsView>
#include <QLabel>

class FieldView : public QGraphicsView {
    Q_OBJECT
public:
    explicit FieldView(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void updateCoordLabel(const QPoint& viewPos);

    QLabel* m_coordLabel = nullptr;
    bool    m_panning    = false;
    bool    m_fittedOnce = false;
    QPoint  m_lastPanPos;
};
