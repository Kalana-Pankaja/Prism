#include "ClipEditDialog.h"
#include "../core/VideoPlayer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QShowEvent>
#include <cmath>

ClipEditDialog::ClipEditDialog(const QString &clipPath, double startTime, double endTime, QWidget *parent)
    : QDialog(parent), m_clipPath(clipPath), m_startTime(startTime), m_endTime(endTime) {
    setWindowTitle("Edit Clip — " + clipPath.section('/', -1));
    setMinimumSize(640, 580);

    {
        VideoPlayer tmp;
        if (tmp.open(clipPath))
            m_duration = tmp.getDuration();
    }
    if (m_duration <= 0.0) m_duration = 1.0;
    if (m_endTime < 0.0 || m_endTime > m_duration) m_endTime = m_duration;
    m_startTime = clampTime(m_startTime);

    pollTimer = new QTimer(this);
    pollTimer->setInterval(80);
    connect(pollTimer, &QTimer::timeout, this, &ClipEditDialog::onPollTimer);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // ── Video preview ──────────────────────────────────────────────────────
    preview = new VideoWidget(this);
    preview->setMinimumHeight(300);
    mainLayout->addWidget(preview, 1);

    // ── Progress bar ───────────────────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0, 4, 0, 0);
        hl->setSpacing(8);

        progressSlider = new QSlider(Qt::Horizontal, this);
        progressSlider->setRange(0, toSliderVal(m_duration));
        progressSlider->setValue(toSliderVal(m_startTime));

        durationLabel = new QLabel("/ " + formatTime(m_duration), this);
        durationLabel->setStyleSheet("font-family: monospace; font-size: 11px; color: #888;");
        durationLabel->setFixedWidth(80);

        hl->addWidget(progressSlider, 1);
        hl->addWidget(durationLabel);
        mainLayout->addWidget(row);
    }

    // ── Navigation + play/pause ────────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(4);

        auto makeNavBtn = [&](const QString &label, double delta) {
            auto *btn = new QPushButton(label, this);
            btn->setFixedWidth(42);
            btn->setFixedHeight(26);
            btn->setStyleSheet("font-size: 10px;");
            connect(btn, &QPushButton::clicked, this, [this, delta]() { seekRelative(delta); });
            return btn;
        };

        hl->addStretch(1);
        hl->addWidget(makeNavBtn("◀10s", -10.0));
        hl->addWidget(makeNavBtn("◀5s",  -5.0));
        hl->addWidget(makeNavBtn("◀1s",  -1.0));
        hl->addWidget(makeNavBtn("◀0.1", -0.1));

        playPauseBtn = new QPushButton("▶", this);
        playPauseBtn->setFixedSize(48, 32);
        playPauseBtn->setStyleSheet("font-size: 14px; font-weight: bold;");
        connect(playPauseBtn, &QPushButton::clicked, this, &ClipEditDialog::onPlayPauseClicked);
        hl->addWidget(playPauseBtn);

        hl->addWidget(makeNavBtn("0.1▶",  0.1));
        hl->addWidget(makeNavBtn("1s▶",   1.0));
        hl->addWidget(makeNavBtn("5s▶",   5.0));
        hl->addWidget(makeNavBtn("10s▶", 10.0));
        hl->addStretch(1);

        mainLayout->addWidget(row);
    }

    // ── Editable current timestamp ─────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(6);

        auto *lbl = new QLabel("Current:", this);
        lbl->setStyleSheet("font-size: 11px; color: #aaa;");

        timestampEdit = new QLineEdit(formatTime(m_startTime), this);
        timestampEdit->setFixedWidth(90);
        timestampEdit->setAlignment(Qt::AlignCenter);
        timestampEdit->setStyleSheet("font-family: monospace; font-size: 13px; padding: 2px 4px;");
        timestampEdit->setPlaceholderText("M:SS.ss");
        timestampEdit->setToolTip("Type a time (M:SS.ss) and press Enter to seek");

        auto *totalLbl = new QLabel("/ " + formatTime(m_duration), this);
        totalLbl->setStyleSheet("font-family: monospace; font-size: 11px; color: #888;");

        hl->addStretch(1);
        hl->addWidget(lbl);
        hl->addWidget(timestampEdit);
        hl->addWidget(totalLbl);
        hl->addStretch(1);

        mainLayout->addWidget(row);
    }

    // ── Set start / Set end buttons ────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0, 4, 0, 0);
        hl->setSpacing(10);

        auto *setStartBtn = new QPushButton("Set Start to Here", this);
        auto *setEndBtn   = new QPushButton("Set End to Here",   this);

        for (auto *btn : {setStartBtn, setEndBtn}) {
            btn->setFixedHeight(28);
            btn->setStyleSheet(
                "QPushButton { background: #1f3d45; border: 1px solid #2a8fa0; border-radius: 5px;"
                " color: #e0e0e0; font-size: 11px; font-weight: bold; }"
                "QPushButton:hover { background: #2a5c6a; }"
                "QPushButton:pressed { background: #163039; }");
        }

        connect(setStartBtn, &QPushButton::clicked, this, &ClipEditDialog::onSetStart);
        connect(setEndBtn,   &QPushButton::clicked, this, &ClipEditDialog::onSetEnd);

        hl->addStretch(1);
        hl->addWidget(setStartBtn);
        hl->addWidget(setEndBtn);
        hl->addStretch(1);

        mainLayout->addWidget(row);
    }

    // ── Current selection display ──────────────────────────────────────────
    {
        auto *group = new QGroupBox("Current Selection", this);
        auto *hl = new QHBoxLayout(group);
        hl->setSpacing(16);

        auto *startHead = new QLabel("Start:", this);
        startHead->setStyleSheet("color: #888; font-size: 11px;");

        startTimeLabel = new QLabel(formatTime(m_startTime), this);
        startTimeLabel->setStyleSheet(
            "font-family: monospace; font-size: 13px; color: #4fc3d0; font-weight: bold;");

        auto *arrow = new QLabel("→", this);
        arrow->setStyleSheet("color: #555; font-size: 14px;");

        auto *endHead = new QLabel("End:", this);
        endHead->setStyleSheet("color: #888; font-size: 11px;");

        endTimeLabel = new QLabel(formatTime(m_endTime), this);
        endTimeLabel->setStyleSheet(
            "font-family: monospace; font-size: 13px; color: #4fc3d0; font-weight: bold;");

        hl->addStretch(1);
        hl->addWidget(startHead);
        hl->addWidget(startTimeLabel);
        hl->addWidget(arrow);
        hl->addWidget(endHead);
        hl->addWidget(endTimeLabel);
        hl->addStretch(1);

        mainLayout->addWidget(group);
    }

    // ── Status label ───────────────────────────────────────────────────────
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setFixedHeight(18);
    statusLabel->setStyleSheet("font-size: 11px;");
    mainLayout->addWidget(statusLabel);

    // ── Dialog buttons ─────────────────────────────────────────────────────
    auto *dlgButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(dlgButtons);

    // ── Connections ────────────────────────────────────────────────────────
    connect(progressSlider, &QSlider::sliderPressed,  this, [this]() { m_sliderDragging = true; });
    connect(progressSlider, &QSlider::sliderMoved,    this, &ClipEditDialog::onProgressSliderMoved);
    connect(progressSlider, &QSlider::sliderReleased, this, &ClipEditDialog::onProgressSliderReleased);
    connect(timestampEdit,  &QLineEdit::editingFinished, this, &ClipEditDialog::onTimestampEditFinished);
    connect(dlgButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dlgButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ClipEditDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!m_videoLoaded) {
        m_videoLoaded = true;
        QTimer::singleShot(60, this, [this]() {
            preview->loadVideo(m_clipPath);

            // Reconcile duration: the VideoWidget's decoder may return a more
            // accurate value than the temp player used in the constructor.
            double d = preview->getDuration();
            if (d > 0.0 && std::fabs(d - m_duration) > 0.5) {
                m_duration = d;
                progressSlider->setRange(0, toSliderVal(m_duration));
                durationLabel->setText("/ " + formatTime(m_duration));
                if (m_endTime > m_duration) {
                    m_endTime = m_duration;
                    updateSelectionLabels();
                }
            }

            seekTo(m_startTime);
            pollTimer->start();
        });
    }
}

// ── Slots ──────────────────────────────────────────────────────────────────

void ClipEditDialog::onPlayPauseClicked() {
    if (!m_videoLoaded) return;
    if (preview->isPlaying()) {
        preview->pause();
        playPauseBtn->setText("▶");
    } else {
        preview->play();
        playPauseBtn->setText("⏸");
    }
}

void ClipEditDialog::onProgressSliderMoved(int value) {
    // Real-time display update while dragging; seek happens on release
    double t = fromSliderVal(value);
    if (!timestampEdit->hasFocus())
        timestampEdit->setText(formatTime(t));
}

void ClipEditDialog::onProgressSliderReleased() {
    m_sliderDragging = false;
    double t = fromSliderVal(progressSlider->value());
    seekTo(t, false); // slider already at position, no need to set it again
}

void ClipEditDialog::onTimestampEditFinished() {
    bool ok = false;
    double t = parseTime(timestampEdit->text(), &ok);
    if (!ok) {
        setStatus("Invalid time format — use M:SS.ss");
        timestampEdit->setText(formatTime(preview->getCurrentTime()));
        return;
    }
    setStatus({});
    seekTo(clampTime(t));
}

void ClipEditDialog::onSetStart() {
    double t = preview->getCurrentTime();
    if (t >= m_endTime - 0.1) {
        setStatus("Start must be at least 0.1 s before end (" + formatTime(m_endTime) + ")");
        return;
    }
    m_startTime = t;
    setStatus({});
    updateSelectionLabels();
}

void ClipEditDialog::onSetEnd() {
    double t = preview->getCurrentTime();
    if (t <= m_startTime + 0.1) {
        setStatus("End must be at least 0.1 s after start (" + formatTime(m_startTime) + ")");
        return;
    }
    if (t > m_duration) {
        setStatus("End cannot exceed clip duration (" + formatTime(m_duration) + ")");
        return;
    }
    m_endTime = t;
    setStatus({});
    updateSelectionLabels();
}

void ClipEditDialog::onPollTimer() {
    if (!m_videoLoaded) return;

    // Sync play/pause button
    playPauseBtn->setText(preview->isPlaying() ? "⏸" : "▶");

    double t = preview->getCurrentTime();

    // Update progress slider (skip if user is dragging)
    if (!m_sliderDragging) {
        progressSlider->blockSignals(true);
        progressSlider->setValue(toSliderVal(t));
        progressSlider->blockSignals(false);
    }

    // Update timestamp edit (skip if user is typing)
    if (!timestampEdit->hasFocus()) {
        timestampEdit->setText(formatTime(t));
    }
}

// ── Helpers ────────────────────────────────────────────────────────────────

void ClipEditDialog::seekRelative(double delta) {
    if (!m_videoLoaded) return;
    double t = clampTime(preview->getCurrentTime() + delta);
    seekTo(t);
}

void ClipEditDialog::seekTo(double secs, bool updateSlider) {
    if (!m_videoLoaded) return;
    secs = clampTime(secs);
    bool wasPlaying = preview->isPlaying();
    if (wasPlaying) preview->pause();
    preview->seek(secs);
    if (wasPlaying) preview->play();

    if (updateSlider) {
        progressSlider->blockSignals(true);
        progressSlider->setValue(toSliderVal(secs));
        progressSlider->blockSignals(false);
    }
    if (!timestampEdit->hasFocus())
        timestampEdit->setText(formatTime(secs));
}

void ClipEditDialog::updateSelectionLabels() {
    startTimeLabel->setText(formatTime(m_startTime));
    endTimeLabel->setText(formatTime(m_endTime));
}

void ClipEditDialog::setStatus(const QString &msg, bool error) {
    statusLabel->setText(msg);
    statusLabel->setStyleSheet(
        msg.isEmpty() ? "font-size: 11px;"
        : error ? "font-size: 11px; color: #e05050;"
                : "font-size: 11px; color: #50c050;");
}

QString ClipEditDialog::formatTime(double secs) const {
    if (secs < 0) secs = 0;
    int m = static_cast<int>(secs) / 60;
    double s = secs - m * 60;
    return QString("%1:%2").arg(m).arg(s, 5, 'f', 2, QChar('0'));
}

double ClipEditDialog::parseTime(const QString &str, bool *ok) const {
    // Accepts "M:SS.ss", "M:SS", or plain seconds "12.5"
    if (ok) *ok = false;
    QString s = str.trimmed();
    double result = 0.0;

    if (s.contains(':')) {
        QStringList parts = s.split(':');
        if (parts.size() != 2) return 0.0;
        bool okM, okS;
        int m = parts[0].toInt(&okM);
        double sec = parts[1].toDouble(&okS);
        if (!okM || !okS || m < 0 || sec < 0 || sec >= 60.0) return 0.0;
        result = m * 60.0 + sec;
    } else {
        bool okV;
        result = s.toDouble(&okV);
        if (!okV || result < 0) return 0.0;
    }

    if (ok) *ok = true;
    return result;
}

double ClipEditDialog::clampTime(double t) const {
    return t < 0.0 ? 0.0 : t > m_duration ? m_duration : t;
}

int ClipEditDialog::toSliderVal(double secs) const {
    return static_cast<int>(secs * 100.0);
}

double ClipEditDialog::fromSliderVal(int val) const {
    return val / 100.0;
}
