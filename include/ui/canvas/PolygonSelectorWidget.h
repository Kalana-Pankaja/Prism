#pragma once
#include <QImage>
#include <QPointF>
#include <QVector>
#include <QWidget>

// Interactive polygon editor for the masking node. Shows a scaled video frame;
// left-click empty space to append a vertex, drag a vertex to move it, and
// right-click a vertex to remove it. Vertices are exposed as normalized [0,1]
// coordinates in insertion order.
class PolygonSelectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit PolygonSelectorWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &frame);
    void setPoints(const QVector<QPointF> &points);   // normalized
    QVector<QPointF> points() const { return m_points; }
    void clearPoints();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override { return QSize(480, 270); }

private:
    QRectF frameRect() const;
    QPointF normToWidget(const QPointF &n) const;
    QPointF widgetToNorm(const QPointF &p) const;
    int hitVertex(const QPointF &p) const;

    QImage m_frame;
    QVector<QPointF> m_points;   // normalized vertices
    int m_dragIndex = -1;
};
