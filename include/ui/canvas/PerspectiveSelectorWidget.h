#pragma once
#include <QImage>
#include <QPointF>
#include <QVector>
#include <QWidget>

// Interactive four-corner (keystone) editor. Shows a scaled video frame with a
// draggable handle at each corner; corners are exposed as normalized [0,1]
// coordinates in order top-left, top-right, bottom-right, bottom-left.
class PerspectiveSelectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit PerspectiveSelectorWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &frame);
    void setCorners(const QVector<QPointF> &corners);   // normalized, 4 points
    QVector<QPointF> corners() const { return m_corners; }
    void resetCorners();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override { return QSize(560, 340); }

private:
    QRectF frameRect() const;
    QPointF normToWidget(const QPointF &n) const;
    QPointF widgetToNorm(const QPointF &p) const;

    QImage m_frame;
    QVector<QPointF> m_corners;   // 4 normalized points
    int m_dragIndex = -1;
    qreal m_zoom = 1.0;   // scales the fitted frame size; wheel-adjustable
};
