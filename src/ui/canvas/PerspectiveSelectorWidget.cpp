#include "ui/canvas/PerspectiveSelectorWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>

namespace {
constexpr qreal kHandleR = 7.0;
}

PerspectiveSelectorWidget::PerspectiveSelectorWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(320, 180);
    setMouseTracking(true);
    resetCorners();
}

void PerspectiveSelectorWidget::setFrame(const QImage &frame) {
    m_frame = frame;
    update();
}

void PerspectiveSelectorWidget::setCorners(const QVector<QPointF> &corners) {
    if (corners.size() == 4) {
        m_corners = corners;
        update();
    }
}

void PerspectiveSelectorWidget::resetCorners() {
    m_corners = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    update();
}

QRectF PerspectiveSelectorWidget::frameRect() const {
    const QRectF area = rect().adjusted(kHandleR, kHandleR, -kHandleR, -kHandleR);
    if (m_frame.isNull()) return area;
    const QSizeF fs = m_frame.size().scaled(area.size().toSize(), Qt::KeepAspectRatio);
    QRectF r(0, 0, fs.width(), fs.height());
    r.moveCenter(area.center());
    return r;
}

QPointF PerspectiveSelectorWidget::normToWidget(const QPointF &n) const {
    const QRectF r = frameRect();
    return {r.x() + n.x() * r.width(), r.y() + n.y() * r.height()};
}

QPointF PerspectiveSelectorWidget::widgetToNorm(const QPointF &p) const {
    const QRectF r = frameRect();
    if (r.width() <= 0 || r.height() <= 0) return {0, 0};
    return {std::clamp((p.x() - r.x()) / r.width(), 0.0, 1.0),
            std::clamp((p.y() - r.y()) / r.height(), 0.0, 1.0)};
}

void PerspectiveSelectorWidget::mousePressEvent(QMouseEvent *e) {
    m_dragIndex = -1;
    qreal best = kHandleR * 2.5;
    for (int i = 0; i < m_corners.size(); ++i) {
        const qreal d = QLineF(e->position(), normToWidget(m_corners[i])).length();
        if (d < best) { best = d; m_dragIndex = i; }
    }
    if (m_dragIndex >= 0) {
        m_corners[m_dragIndex] = widgetToNorm(e->position());
        update();
    }
}

void PerspectiveSelectorWidget::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragIndex < 0) return;
    m_corners[m_dragIndex] = widgetToNorm(e->position());
    update();
}

void PerspectiveSelectorWidget::mouseReleaseEvent(QMouseEvent *) {
    m_dragIndex = -1;
}

void PerspectiveSelectorWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(24, 24, 28));

    const QRectF fr = frameRect();
    if (!m_frame.isNull())
        p.drawImage(fr, m_frame);
    else {
        p.setPen(QColor(90, 90, 96));
        p.drawText(fr, Qt::AlignCenter, "No preview");
    }

    QPolygonF poly;
    for (const QPointF &c : m_corners)
        poly << normToWidget(c);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(100, 180, 255), 2));
    p.setBrush(QColor(100, 180, 255, 40));
    p.drawPolygon(poly);

    p.setBrush(QColor(100, 180, 255));
    for (const QPointF &pt : poly)
        p.drawEllipse(pt, kHandleR, kHandleR);
}
