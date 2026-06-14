#pragma once

#include <QMainWindow>
#include "ui/ClipCard.h"
#include "ui/OutputWindow.h"
#include "core/ClipManager.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onClipGridClicked(int index);
    void onLoadFolderClicked();
    void onCrossfaderMoved(int value);
    void onADeckPlayClicked();
    void onBDeckPlayClicked();
    void onADeckSpeedChanged(int value);
    void onBDeckSpeedChanged(int value);
    void onTimerUpdate();
    void onAButtonClicked(int index);
    void onBButtonClicked(int index);

private:
    static constexpr int CARD_WIDTH = 122;
    static constexpr int MIN_COLS = 2;
    int dynamicCols = 8;

    Ui::MainWindow *ui;
    OutputWindow *outputWindow = nullptr;
    ClipCard *clipCards[512];

    bool m_aSliderDragging = false;
    bool m_bSliderDragging = false;

    ClipManager clipManager;
    QTimer *updateTimer = nullptr;
    int selectedClipIndex = -1;
    int aClipIndex = -1;
    int bClipIndex = -1;

    void setupConnections();
    void applyTheme();
    void updateGridLayout();

    static QString formatTimeShort(double secs);
};
