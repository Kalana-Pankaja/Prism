#pragma once

#include <QDialog>
#include <QVector>
#include <QRectF>

class ClipNodeEditor;
class TransformCanvasWidget;
class QListWidget;
class QSpinBox;
class QComboBox;
class QGroupBox;
class QToolButton;

/// Visual editor for the placement of the inputs feeding a Layer node. Edits
/// (move/resize/show/hide/canvas size) are applied to the ClipNodeEditor — and
/// therefore the live output — immediately; Cancel restores the state the
/// dialog opened with.
class TransformEditorDialog : public QDialog {
    Q_OBJECT

public:
    TransformEditorDialog(int contextId, ClipNodeEditor *editor, QWidget *parent = nullptr);

    void reject() override;   // Cancel / Esc / close: revert live edits

private:
    struct Snapshot {
        int    slotIndex = 0;
        QRectF rect;
        bool   visible = true;
    };

    QWidget *buildSidebar();
    void populate();
    void applyRectLive(int canvasIndex);
    void pushSelectedRect(const QRectF &rect);
    void syncPlacementSpins();
    void onCanvasSelectionChanged(int canvasIndex);
    void onListRowChanged(int row);
    void onPlacementSpinEdited();
    void onCanvasSizeEdited();
    void revertAll();

    int rowForCanvasIndex(int canvasIndex) const { return m_clipCount - 1 - canvasIndex; }
    int canvasIndexForRow(int row) const { return m_clipCount - 1 - row; }

    int m_contextId;
    ClipNodeEditor *m_editor;
    TransformCanvasWidget *m_canvas = nullptr;
    QListWidget *m_layerList = nullptr;
    QGroupBox *m_placementGroup = nullptr;
    QSpinBox *m_xSpin = nullptr, *m_ySpin = nullptr, *m_wSpin = nullptr, *m_hSpin = nullptr;
    QSpinBox *m_canvasWSpin = nullptr, *m_canvasHSpin = nullptr;
    QVector<QToolButton *> m_eyeButtons;   // per list row

    QVector<Snapshot> m_snapshot;
    int m_origCanvasW = 1280, m_origCanvasH = 720;
    int m_clipCount = 0;
    bool m_updating = false;
};
