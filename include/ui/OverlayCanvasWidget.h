#pragma once

#include <QWidget>
#include <QImage>
#include <QHash>
#include <QPixmap>
#include <array>
#include "core/OverlayItem.h"

// Interactive canvas for positioning and resizing overlays.
// Shows the current video frame behind all overlays.
// Click to select, drag body to move, drag corner handles to resize.
class OverlayCanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayCanvasWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &frame);
    void setOverlays(const QList<OverlayItem> &overlays);
    void setSelectedIndex(int idx);
    int  selectedIndex() const { return m_selectedIdx; }

signals:
    void overlaySelected(int index);                         // -1 = deselected
    void overlayChanged(int index, const OverlayItem &item); // moved or resized

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override { return QSize(480, 270); }

private:
    enum class DragMode { None, Move, ResizeTL, ResizeTR, ResizeBR, ResizeBL };

    struct HitResult { int index; DragMode mode; };

    QImage  m_frame;
    QList<OverlayItem> m_overlays;
    mutable QHash<QString, QPixmap> m_pixCache;

    int       m_selectedIdx = -1;
    DragMode  m_dragMode    = DragMode::None;
    QPointF   m_dragOrigin;
    QRectF    m_dragRect;       // widget-space rect at drag start
    OverlayItem m_dragItem;     // overlay state at drag start

    QRectF  frameRect() const;
    QRectF  ovToWidget(const OverlayItem &ov) const;
    OverlayItem applyWidgetRect(OverlayItem base, const QRectF &r) const;
    std::array<QRectF, 4> handles(const OverlayItem &ov) const;
    HitResult hitTest(QPointF pt) const;
    void drawOverlay(QPainter &p, const OverlayItem &ov, bool selected) const;
};
