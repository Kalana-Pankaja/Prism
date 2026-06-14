#include "ui/MainWindow.h"
#include "ui_MainWindow.h"
#include "ui/VideoWidget.h"
#include "core/ThumbnailExtractor.h"
#include <QApplication>
#include <QFileDialog>
#include <QTimer>
#include <QDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setAcceptDrops(true);

    outputWindow = new OutputWindow(this);
    outputWindow->show();

    setupConnections();
    applyTheme();

    // Build the empty-state placeholder (shown before any media is loaded)
    m_emptyPlaceholder = new QWidget(ui->gridWidget);
    m_emptyPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *phLayout = new QVBoxLayout(m_emptyPlaceholder);
    phLayout->setAlignment(Qt::AlignCenter);
    phLayout->setSpacing(16);

    auto *phIcon = new QLabel("📁", m_emptyPlaceholder);
    phIcon->setAlignment(Qt::AlignCenter);
    phIcon->setStyleSheet("font-size: 52px;");

    auto *phText = new QLabel("No media loaded\nLoad a folder, add another folder, or pick individual files.",
                              m_emptyPlaceholder);
    phText->setAlignment(Qt::AlignCenter);
    phText->setWordWrap(true);
    phText->setStyleSheet("color: #555; font-size: 13px;");

    // Quick-action buttons embedded in the placeholder
    auto *phBtnRow = new QWidget(m_emptyPlaceholder);
    auto *phBtns   = new QHBoxLayout(phBtnRow);
    phBtns->setSpacing(10);
    auto *phLoad  = new QPushButton("📁  Load Folder",  phBtnRow);
    auto *phAdd   = new QPushButton("📂  Add Folder",   phBtnRow);
    auto *phFiles = new QPushButton("＋  Add Files",    phBtnRow);
    for (auto *b : {phLoad, phAdd, phFiles}) {
        b->setStyleSheet("font-size: 12px; padding: 6px 16px;");
        phBtns->addWidget(b);
    }
    connect(phLoad,  &QPushButton::clicked, this, &MainWindow::onLoadFolderClicked);
    connect(phAdd,   &QPushButton::clicked, this, &MainWindow::onAddFolderClicked);
    connect(phFiles, &QPushButton::clicked, this, &MainWindow::onAddFilesClicked);

    phLayout->addWidget(phIcon);
    phLayout->addWidget(phText);
    phLayout->addWidget(phBtnRow);

    ui->gridLayout->addWidget(m_emptyPlaceholder, 0, 0, 1, 1);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::onTimerUpdate);
    updateTimer->start(100);

    qDebug() << "SwitchX initialized - Live Media Control Mode";
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupConnections() {
    connect(ui->loadFolderBtn,    &QPushButton::clicked,  this, &MainWindow::onLoadFolderClicked);
    connect(ui->addFolderBtn,     &QPushButton::clicked,  this, &MainWindow::onAddFolderClicked);
    connect(ui->addFilesBtn,      &QPushButton::clicked,  this, &MainWindow::onAddFilesClicked);
    connect(ui->clearAllBtn,      &QPushButton::clicked,  this, &MainWindow::onClearAllClicked);
    connect(ui->aDeckPlayBtn,     &QPushButton::clicked,  this, &MainWindow::onADeckPlayClicked);
    connect(ui->bDeckPlayBtn,     &QPushButton::clicked,  this, &MainWindow::onBDeckPlayClicked);
    connect(ui->aDeckSpeedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onADeckSpeedChanged);
    connect(ui->bDeckSpeedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBDeckSpeedChanged);
    connect(ui->crossfaderSlider, &QSlider::valueChanged, this, &MainWindow::onCrossfaderMoved);

    connect(ui->aProgressSlider, &QSlider::sliderPressed, this, [this]() { m_aSliderDragging = true; });
    connect(ui->aProgressSlider, &QSlider::sliderReleased, this, [this]() {
        m_aSliderDragging = false;
        VideoWidget *out = outputWindow->videoWidget();
        double dur = out->getDurationA();
        if (dur > 0) {
            double t = ui->aProgressSlider->value() / 1000.0 * dur;
            out->seekA(t);
        }
    });
    connect(ui->aProgressSlider, &QSlider::sliderMoved, this, [this](int value) {
        VideoWidget *out = outputWindow->videoWidget();
        double dur = out->getDurationA();
        if (dur > 0)
            ui->aTimeLabel->setText(formatTimeShort(value / 1000.0 * dur) + " / " + formatTimeShort(dur));
    });

    connect(ui->bProgressSlider, &QSlider::sliderPressed, this, [this]() { m_bSliderDragging = true; });
    connect(ui->bProgressSlider, &QSlider::sliderReleased, this, [this]() {
        m_bSliderDragging = false;
        VideoWidget *out = outputWindow->videoWidget();
        double dur = out->getDurationB();
        if (dur > 0) {
            double t = ui->bProgressSlider->value() / 1000.0 * dur;
            out->seekB(t);
        }
    });
    connect(ui->bProgressSlider, &QSlider::sliderMoved, this, [this](int value) {
        VideoWidget *out = outputWindow->videoWidget();
        double dur = out->getDurationB();
        if (dur > 0)
            ui->bTimeLabel->setText(formatTimeShort(value / 1000.0 * dur) + " / " + formatTimeShort(dur));
    });
}

void MainWindow::onClipGridClicked(int index) {
    if (index < 0 || index >= m_clipCards.size()) return;
    ClipCard *card = m_clipCards[index];
    if (!card || card->clipPath().isEmpty()) return;

    for (ClipCard *c : m_clipCards) c->setActive(false);
    selectedClipIndex = index;
    card->setActive(true);

    if (ui->crossfaderSlider->value() <= 50)
        onAButtonClicked(index);
    else
        onBButtonClicked(index);
}

void MainWindow::onLoadFolderClicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Media Folder");
    if (path.isEmpty()) return;
    clipManager.loadFolder(path);
    selectedClipIndex = aClipIndex = bClipIndex = -1;
    rebuildGrid();
}

void MainWindow::onAddFolderClicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Add Media Folder");
    if (path.isEmpty()) return;
    clipManager.addFolder(path);
    rebuildGrid();
}

void MainWindow::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Add Media Files", "",
        "Media Files (*.mp4 *.avi *.mov *.mkv *.webm *.png *.jpg *.jpeg)");
    if (files.isEmpty()) return;
    clipManager.addFiles(files);
    rebuildGrid();
}

void MainWindow::onClearAllClicked() {
    clipManager.clear();
    selectedClipIndex = aClipIndex = bClipIndex = -1;
    rebuildGrid();
}

// ── Grid management ───────────────────────────────────────────────────────────

void MainWindow::rebuildGrid() {
    // Remove every item from the layout
    while (QLayoutItem *item = ui->gridLayout->takeAt(0)) {
        if (item->widget()) item->widget()->hide();
        delete item;
    }

    // Delete all old ClipCards
    for (ClipCard *c : m_clipCards) c->deleteLater();
    m_clipCards.clear();

    int n = clipManager.getClipCount();

    if (n == 0) {
        // Show the "no media" placeholder
        m_emptyPlaceholder->show();
        ui->gridLayout->addWidget(m_emptyPlaceholder, 0, 0, 1, 1);
        return;
    }

    m_emptyPlaceholder->hide();

    // Create exactly n ClipCards
    int availW = ui->clipsScrollArea->viewport()->width();
    if (availW < 10) availW = width() - 40;
    dynamicCols = std::max(MIN_COLS, availW / (CARD_WIDTH + 4));

    for (int i = 0; i < n; ++i) {
        auto *card = new ClipCard(i, ui->gridWidget);
        connect(card, &ClipCard::triggered,      this, &MainWindow::onClipGridClicked);
        connect(card, &ClipCard::aButtonClicked, this, &MainWindow::onAButtonClicked);
        connect(card, &ClipCard::bButtonClicked, this, &MainWindow::onBButtonClicked);

        QString path  = clipManager.getClipPath(i);
        QPixmap thumb = ThumbnailExtractor::extract(path, 110, 65);
        card->loadClip(path, thumb);

        ui->gridLayout->addWidget(card, i / dynamicCols, i % dynamicCols);
        card->show();
        m_clipCards.append(card);
    }

    qDebug() << "Grid rebuilt with" << n << "clips";
}

void MainWindow::onCrossfaderMoved(int value) {
    outputWindow->videoWidget()->setShowA(value <= 50);
}

void MainWindow::onADeckPlayClicked() {
    VideoWidget *output = outputWindow->videoWidget();
    if (output->isPlayingA())
        output->pauseA();
    else
        output->playA();
}

void MainWindow::onBDeckPlayClicked() {
    VideoWidget *output = outputWindow->videoWidget();
    if (output->isPlayingB())
        output->pauseB();
    else
        output->playB();
}

void MainWindow::onADeckSpeedChanged(int value) {
    qDebug() << "A Deck Speed:" << value << "%";
}

void MainWindow::onBDeckSpeedChanged(int value) {
    qDebug() << "B Deck Speed:" << value << "%";
}

void MainWindow::onTimerUpdate() {
    VideoWidget *out = outputWindow->videoWidget();

    double durA = out->getDurationA();
    double timeA = out->getCurrentTimeA();
    if (durA > 0) {
        if (!m_aSliderDragging) {
            ui->aProgressSlider->blockSignals(true);
            ui->aProgressSlider->setValue((int)(timeA / durA * 1000));
            ui->aProgressSlider->blockSignals(false);
        }
        ui->aTimeLabel->setText(formatTimeShort(timeA) + " / " + formatTimeShort(durA));
    }
    ui->aDeckPlayBtn->setText(out->isPlayingA() ? "⏸" : "▶");

    QImage frameA = out->getFrameA();
    if (!frameA.isNull())
        ui->aPreviewLabel->setPixmap(QPixmap::fromImage(
            frameA.scaled(160, 90, Qt::KeepAspectRatio, Qt::FastTransformation)));

    double durB = out->getDurationB();
    double timeB = out->getCurrentTimeB();
    if (durB > 0) {
        if (!m_bSliderDragging) {
            ui->bProgressSlider->blockSignals(true);
            ui->bProgressSlider->setValue((int)(timeB / durB * 1000));
            ui->bProgressSlider->blockSignals(false);
        }
        ui->bTimeLabel->setText(formatTimeShort(timeB) + " / " + formatTimeShort(durB));
    }
    ui->bDeckPlayBtn->setText(out->isPlayingB() ? "⏸" : "▶");

    QImage frameB = out->getFrameB();
    if (!frameB.isNull())
        ui->bPreviewLabel->setPixmap(QPixmap::fromImage(
            frameB.scaled(160, 90, Qt::KeepAspectRatio, Qt::FastTransformation)));
}

void MainWindow::updateGridLayout() {
    if (m_clipCards.isEmpty()) return;

    while (QLayoutItem *item = ui->gridLayout->takeAt(0)) {
        if (item->widget()) item->widget()->hide();
        delete item;
    }

    int availW = ui->clipsScrollArea->viewport()->width();
    if (availW < 10) availW = width() - 40;
    dynamicCols = std::max(MIN_COLS, availW / (CARD_WIDTH + 4));

    for (int i = 0; i < m_clipCards.size(); ++i) {
        m_clipCards[i]->show();
        ui->gridLayout->addWidget(m_clipCards[i], i / dynamicCols, i % dynamicCols);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateGridLayout();
}

void MainWindow::applyTheme() {
    QString styleSheet = R"(
        QMainWindow, QDialog, QWidget {
            background-color: #242528;
            color: #E0E0E0;
            font-family: "Segoe UI", Arial, sans-serif;
            font-size: 13px;
        }

        QGroupBox {
            background-color: #242528;
            border: 1px solid #1c1d1f;
            border-radius: 12px;
            padding-top: 10px;
            margin-top: 10px;
            color: #E0E0E0;
            font-weight: bold;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
            color: #2a8fa0;
            font-weight: bold;
            font-size: 12px;
        }

        QPushButton {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2a2c30, stop:1 #1e1f22);
            color: #E0E0E0;
            border-top: 1px solid #33363b;
            border-left: 1px solid #33363b;
            border-bottom: 1px solid #151618;
            border-right: 1px solid #151618;
            border-radius: 6px;
            padding: 4px 10px;
            font-weight: bold;
            font-size: 11px;
            min-height: 22px;
            height: 22px;
        }

        QPushButton:hover {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2e3136, stop:1 #222326);
            color: #FFFFFF;
        }

        QPushButton:pressed {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #191a1c, stop:1 #2b2d32);
            border-top: 1px solid #121314;
            border-left: 1px solid #121314;
            border-bottom: 1px solid #3a3d43;
            border-right: 1px solid #3a3d43;
            color: #aaaaaa;
        }

        QPushButton#accentButton, QPushButton[text*="Load"], QPushButton[text*="Play"], QPushButton[text*="Fullscreen"] {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3a6670, stop:1 #1f3d45);
            color: #FFFFFF;
            border-top: 1px solid #4a7f8c;
            border-left: 1px solid #4a7f8c;
            border-bottom: 1px solid #112226;
            border-right: 1px solid #112226;
            padding: 4px 12px;
            font-size: 11px;
            min-height: 22px;
            height: 22px;
        }

        QPushButton#accentButton:pressed, QPushButton[text*="Play"]:pressed {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #15292e, stop:1 #2f545c);
            border-top: 1px solid #0f1d21;
            border-left: 1px solid #0f1d21;
        }

        QScrollBar:vertical {
            background-color: #1c1d1f;
            width: 12px;
            margin: 0px;
            border-radius: 6px;
        }

        QScrollBar::handle:vertical {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #32353a, stop:1 #242528);
            min-height: 20px;
            border-radius: 6px;
            border: 1px solid #151618;
        }

        QScrollBar::handle:vertical:hover { background-color: #3d4147; }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            background: none;
            height: 0px;
        }

        QScrollBar:horizontal {
            background-color: #1c1d1f;
            height: 12px;
            margin: 0px;
            border-radius: 6px;
        }

        QScrollBar::handle:horizontal {
            background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #32353a, stop:1 #242528);
            min-width: 20px;
            border-radius: 6px;
            border: 1px solid #151618;
        }

        QScrollBar::handle:horizontal:hover { background-color: #3d4147; }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            background: none;
            width: 0px;
        }

        QSlider::groove:horizontal {
            border: 1px solid #1c1d1f;
            height: 6px;
            background: #18191b;
            border-radius: 3px;
        }

        QSlider::sub-page:horizontal {
            background: #2a5c66;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3a3d43, stop:1 #1c1d1f);
            border: 1px solid #4a4e56;
            width: 14px;
            margin-top: -5px;
            margin-bottom: -5px;
            border-radius: 7px;
        }

        QSlider::handle:horizontal:hover { background: #4a4e56; }

        QSlider::groove:vertical {
            border: 1px solid #1c1d1f;
            width: 6px;
            background: #18191b;
            border-radius: 3px;
        }

        QSlider::handle:vertical {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3a3d43, stop:1 #1c1d1f);
            border: 1px solid #4a4e56;
            height: 14px;
            margin-left: -5px;
            margin-right: -5px;
            border-radius: 7px;
        }

        QLabel {
            color: #E0E0E0;
            background-color: transparent;
            font-size: 12px;
        }

        QSpinBox {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2a2c30, stop:1 #1e1f22);
            color: #E0E0E0;
            border-top: 1px solid #33363b;
            border-left: 1px solid #33363b;
            border-bottom: 1px solid #151618;
            border-right: 1px solid #151618;
            border-radius: 6px;
            padding: 4px;
            selection-background-color: #2a5c66;
        }

        QSpinBox:hover {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2e3136, stop:1 #222326);
        }

        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #1e1f22;
            border: 1px solid #151618;
            border-radius: 3px;
            width: 16px;
        }

        QSpinBox::up-button:pressed, QSpinBox::down-button:pressed {
            background-color: #151618;
        }

        QListWidget {
            background-color: #1c1d1f;
            border: 1px solid #151618;
            border-radius: 8px;
            color: #E0E0E0;
        }

        QListWidget::item { padding: 4px; }
        QListWidget::item:selected { background-color: #2a5c66; color: #FFFFFF; }
        QListWidget::item:hover { background-color: #2a2c30; }
    )";
    qApp->setStyle("fusion");
    qApp->setStyleSheet(styleSheet);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls()) return;

    QStringList paths;
    for (const QUrl &url : mimeData->urls()) {
        QString p = url.toLocalFile();
        QFileInfo fi(p);
        if (fi.isDir()) {
            clipManager.addFolder(p);
        } else {
            paths << p;
        }
    }
    if (!paths.isEmpty())
        clipManager.addFiles(paths);
    rebuildGrid();
    event->acceptProposedAction();
}

void MainWindow::onAButtonClicked(int index) {
    if (index < 0 || index >= m_clipCards.size()) return;
    ClipCard *card = m_clipCards[index];
    if (!card || card->clipPath().isEmpty()) return;

    if (aClipIndex >= 0 && aClipIndex < m_clipCards.size())
        m_clipCards[aClipIndex]->setASelected(false);
    aClipIndex = index;
    card->setASelected(true);

    VideoWidget *out = outputWindow->videoWidget();
    out->setRepeatA(card->isRepeat());
    out->setTrimPointsA(card->startTime(), card->endTime());
    out->setCropA(card->cropX(), card->cropY(), card->cropW(), card->cropH());
    out->setOverlaysA(card->overlays());
    out->loadVideoA(card->clipPath());
    if (card->startTime() > 0) out->seekA(card->startTime());
    out->playA();

    double dur = out->getDurationA();
    ui->aProgressSlider->setRange(0, 1000);
    ui->aProgressSlider->setValue(0);
    ui->aProgressSlider->setEnabled(true);
    ui->aDeckPlayBtn->setEnabled(true);
    ui->aSelectedLabel->setText(QString("A: %1").arg(QFileInfo(card->clipPath()).baseName()));
    ui->aTimeLabel->setText(formatTimeShort(card->startTime()) + " / " + formatTimeShort(dur));
}

void MainWindow::onBButtonClicked(int index) {
    if (index < 0 || index >= m_clipCards.size()) return;
    ClipCard *card = m_clipCards[index];
    if (!card || card->clipPath().isEmpty()) return;

    if (bClipIndex >= 0 && bClipIndex < m_clipCards.size())
        m_clipCards[bClipIndex]->setBSelected(false);
    bClipIndex = index;
    card->setBSelected(true);

    VideoWidget *out = outputWindow->videoWidget();
    out->setRepeatB(card->isRepeat());
    out->setTrimPointsB(card->startTime(), card->endTime());
    out->setCropB(card->cropX(), card->cropY(), card->cropW(), card->cropH());
    out->setOverlaysB(card->overlays());
    out->loadVideoB(card->clipPath());
    if (card->startTime() > 0) out->seekB(card->startTime());
    out->playB();

    double dur = out->getDurationB();
    ui->bProgressSlider->setRange(0, 1000);
    ui->bProgressSlider->setValue(0);
    ui->bProgressSlider->setEnabled(true);
    ui->bDeckPlayBtn->setEnabled(true);
    ui->bSelectedLabel->setText(QString("B: %1").arg(QFileInfo(card->clipPath()).baseName()));
    ui->bTimeLabel->setText(formatTimeShort(card->startTime()) + " / " + formatTimeShort(dur));
}

QString MainWindow::formatTimeShort(double secs) {
    if (secs < 0) secs = 0;
    int m = (int)secs / 60;
    int s = (int)secs % 60;
    return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}
