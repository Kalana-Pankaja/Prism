#include "ui/CropSelectorWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

CropSelectorWidget::CropSelectorWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setMinimumSize(200, 112);
}

void CropSelectorWidget::setFrame(const QImage &frame) {
    m_frame = frame;
    update();
}

void CropSelectorWidget::setCrop(float x, float y, float w, float h) {
    m_cx = x; m_cy = y; m_cw = w; m_ch = h;
    update();
}

// Returns the rectangle inside the widget where the frame is drawn (letterboxed).
QRectF CropSelectorWidget::frameRect() const {
    if (m_frame.isNull()) return rect();
    double fw = m_frame.width(), fh = m_frame.height();
    double ww = width(), wh = height();
    double scale = std::min(ww / fw, wh / fh);
    double dw = fw * scale, dh = fh * scale;
    return QRectF((ww - dw) / 2.0, (wh - dh) / 2.0, dw, dh);
}

// Widget point → normalized [0,1] clamped to frameRect.
QPointF CropSelectorWidget::toNorm(QPointF pt) const {
    QRectF fr = frameRect();
    if (fr.isEmpty()) return {};
    double nx = std::clamp((pt.x() - fr.left()) / fr.width(),  0.0, 1.0);
    double ny = std::clamp((pt.y() - fr.top())  / fr.height(), 0.0, 1.0);
    return {nx, ny};
}

// Current crop rect in widget coordinates.
QRectF CropSelectorWidget::cropInWidget() const {
    QRectF fr = frameRect();
    return QRectF(
        fr.left() + m_cx * fr.width(),
        fr.top()  + m_cy * fr.height(),
        m_cw * fr.width(),
        m_ch * fr.height()
    );
}

void CropSelectorWidget::commitDrag() {
    QPointF a = toNorm(m_p0), b = toNorm(m_p1);
    float x0 = std::min((float)a.x(), (float)b.x());
    float y0 = std::min((float)a.y(), (float)b.y());
    float x1 = std::max((float)a.x(), (float)b.x());
    float y1 = std::max((float)a.y(), (float)b.y());
    float w = std::max(0.01f, x1 - x0);
    float h = std::max(0.01f, y1 - y0);
    m_cx = x0; m_cy = y0; m_cw = w; m_ch = h;
    emit cropChanged(m_cx, m_cy, m_cw, m_ch);
    update();
}

void CropSelectorWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_p0 = m_p1 = e->position();
    }
}

void CropSelectorWidget::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging) {
        m_p1 = e->position();
        update();
    }
}

void CropSelectorWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (m_dragging && e->button() == Qt::LeftButton) {
        m_p1 = e->position();
        m_dragging = false;
        // If drag was too small, treat as a click → reset
        QPointF d = m_p1 - m_p0;
        if (std::abs(d.x()) < 4 && std::abs(d.y()) < 4) {
            m_cx = 0.f; m_cy = 0.f; m_cw = 1.f; m_ch = 1.f;
            emit cropChanged(m_cx, m_cy, m_cw, m_ch);
            update();
        } else {
            commitDrag();
        }
    }
}

void CropSelectorWidget::mouseDoubleClickEvent(QMouseEvent *) {
    m_cx = 0.f; m_cy = 0.f; m_cw = 1.f; m_ch = 1.f;
    emit cropChanged(m_cx, m_cy, m_cw, m_ch);
    update();
}

void CropSelectorWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0x18, 0x19, 0x1b));

    QRectF fr = frameRect();

    // Draw the frame
    if (!m_frame.isNull())
        p.drawImage(fr, m_frame);
    else {
        p.setPen(QColor(0x33, 0x36, 0x3b));
        p.drawRect(fr);
        p.setPen(QColor(0x88, 0x88, 0x88));
        p.drawText(fr, Qt::AlignCenter, "No frame — press Play then switch here");
    }

    // Dim everything outside the crop rect
    QRectF cr = m_dragging ? [&]() {
        QPointF a = toNorm(m_p0), b = toNorm(m_p1);
        float x0 = std::min((float)a.x(), (float)b.x());
        float y0 = std::min((float)a.y(), (float)b.y());
        float x1 = std::max((float)a.x(), (float)b.x());
        float y1 = std::max((float)a.y(), (float)b.y());
        return QRectF(
            fr.left() + x0 * fr.width(), fr.top() + y0 * fr.height(),
            (x1 - x0) * fr.width(),      (y1 - y0) * fr.height());
    }() : cropInWidget();

    if (!m_frame.isNull()) {
        p.setOpacity(0.55);
        p.fillRect(QRectF(fr.left(), fr.top(), fr.width(), cr.top() - fr.top()), Qt::black);
        p.fillRect(QRectF(fr.left(), cr.bottom(), fr.width(), fr.bottom() - cr.bottom()), Qt::black);
        p.fillRect(QRectF(fr.left(), cr.top(), cr.left() - fr.left(), cr.height()), Qt::black);
        p.fillRect(QRectF(cr.right(), cr.top(), fr.right() - cr.right(), cr.height()), Qt::black);
        p.setOpacity(1.0);
    }

    // Crop border
    p.setPen(QPen(QColor(0x2a, 0x8f, 0xa0), 1.5, Qt::SolidLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(cr);

    // Corner handles
    const double hs = 6;
    p.setBrush(QColor(0x4f, 0xc3, 0xd0));
    p.setPen(Qt::NoPen);
    for (QPointF corner : {cr.topLeft(), cr.topRight(), cr.bottomLeft(), cr.bottomRight()})
        p.drawRect(QRectF(corner.x() - hs/2, corner.y() - hs/2, hs, hs));

    // Hint text
    p.setPen(QColor(0x55, 0x55, 0x55));
    p.setFont(QFont("Segoe UI", 9));
    p.drawText(QRectF(0, height() - 18, width(), 18),
               Qt::AlignCenter, "Drag to crop  ·  Double-click to reset");
}
