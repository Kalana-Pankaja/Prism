#pragma once

#include <QWidget>
#include <QVector>
#include <QPixmap>
#include <QRectF>
#include <array>

struct ClipItem {
    int clipId = 0;
    QPixmap thumbnail;
    QRectF rect;    // normalized [0,1]
    bool selected = false;
    bool visible = true;
    QString name;
};

/// Interactive canvas that lays out multiple clips inside a fixed-aspect logical
/// canvas: select, drag to move, drag corner/edge handles to resize, with
/// edge/center snapping (Alt disables), Shift aspect lock and arrow-key nudging.
/// Coordinates are normalized [0,1]; see ui/canvas/CanvasGeometry.h.
class TransformCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit TransformCanvasWidget(QWidget *parent = nullptr);

    void setCanvasSize(int w, int h);
    int canvasW() const { return m_canvasW; }
    int canvasH() const { return m_canvasH; }

    void setClips(const QVector<ClipItem> &clips);
    QVector<ClipItem> getClips() const { return m_clips; }
    void clear();

    int selectedIndex() const;
    void setSelectedIndex(int index);                  // programmatic, no signal
    void setClipRect(int index, const QRectF &rect);   // clamped, no signal
    void setClipVisible(int index, bool visible);

signals:
    void clipRectChanged(int index);   // interactive move/resize/nudge
    void selectionChanged(int index);  // -1 = none

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    enum class Handle { None, Move, N, S, E, W, NW, NE, SW, SE };

    QRectF canvasRect() const;
    QRectF itemToScreen(const QRectF &itemRect) const;

    // Order: NW, N, NE, E, SE, S, SW, W.
    std::array<QRectF, 8> handleRects(const QRectF &screenRect, double size) const;
    Handle handleAt(const QPointF &pos, int *itemIndex) const;
    int itemAt(const QPointF &pos) const;

    QRectF clampMove(QRectF r) const;
    void collectSnapTargets(QVector<qreal> &xs, QVector<qreal> &ys) const;
    void snapMove(QRectF &r);
    void snapResize(QRectF &r, Handle h);
    void updateCursor(const QPointF &pos);
    void selectOnly(int index);   // user-driven, emits selectionChanged

    int m_canvasW = 1280;
    int m_canvasH = 720;
    QVector<ClipItem> m_clips;

    int m_dragIndex = -1;
    Handle m_dragHandle = Handle::None;
    QPointF m_pressPos;
    QRectF m_origRect;
    bool m_dragging = false;

    QVector<qreal> m_guidesX, m_guidesY;   // active snap guides, normalized
};
