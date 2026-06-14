#include "ui/ClipEditDialog.h"
#include "ui_ClipEditDialog.h"
#include "core/VideoPlayer.h"
#include <QShowEvent>
#include <cmath>

ClipEditDialog::ClipEditDialog(const QString &clipPath, double startTime, double endTime, QWidget *parent)
    : QDialog(parent), ui(new Ui::ClipEditDialog),
      m_clipPath(clipPath), m_startTime(startTime), m_endTime(endTime) {
    ui->setupUi(this);
    setWindowTitle("Edit Clip — " + clipPath.section('/', -1));

    {
        VideoPlayer tmp;
        if (tmp.open(clipPath))
            m_duration = tmp.getDuration();
    }
    if (m_duration <= 0.0) m_duration = 1.0;
    if (m_endTime < 0.0 || m_endTime > m_duration) m_endTime = m_duration;
    m_startTime = clampTime(m_startTime);

    ui->progressSlider->setRange(0, toSliderVal(m_duration));
    ui->progressSlider->setValue(toSliderVal(m_startTime));
    ui->durationLabel->setText("/ " + formatTime(m_duration));
    ui->timestampEdit->setText(formatTime(m_startTime));
    ui->totalDurationLabel->setText("/ " + formatTime(m_duration));
    updateSelectionLabels();

    pollTimer = new QTimer(this);
    pollTimer->setInterval(80);
    connect(pollTimer, &QTimer::timeout, this, &ClipEditDialog::onPollTimer);

    connect(ui->playPauseBtn,  &QPushButton::clicked, this, &ClipEditDialog::onPlayPauseClicked);
    connect(ui->setStartBtn,   &QPushButton::clicked, this, &ClipEditDialog::onSetStart);
    connect(ui->setEndBtn,     &QPushButton::clicked, this, &ClipEditDialog::onSetEnd);

    connect(ui->btn10sBack,    &QPushButton::clicked, this, [this]() { seekRelative(-10.0); });
    connect(ui->btn5sBack,     &QPushButton::clicked, this, [this]() { seekRelative(-5.0); });
    connect(ui->btn1sBack,     &QPushButton::clicked, this, [this]() { seekRelative(-1.0); });
    connect(ui->btn01sBack,    &QPushButton::clicked, this, [this]() { seekRelative(-0.1); });
    connect(ui->btn01sForward, &QPushButton::clicked, this, [this]() { seekRelative(0.1); });
    connect(ui->btn1sForward,  &QPushButton::clicked, this, [this]() { seekRelative(1.0); });
    connect(ui->btn5sForward,  &QPushButton::clicked, this, [this]() { seekRelative(5.0); });
    connect(ui->btn10sForward, &QPushButton::clicked, this, [this]() { seekRelative(10.0); });

    connect(ui->progressSlider, &QSlider::sliderPressed,  this, [this]() { m_sliderDragging = true; });
    connect(ui->progressSlider, &QSlider::sliderMoved,    this, &ClipEditDialog::onProgressSliderMoved);
    connect(ui->progressSlider, &QSlider::sliderReleased, this, &ClipEditDialog::onProgressSliderReleased);
    connect(ui->timestampEdit,  &QLineEdit::editingFinished, this, &ClipEditDialog::onTimestampEditFinished);
}

ClipEditDialog::~ClipEditDialog() {
    delete ui;
}

void ClipEditDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!m_videoLoaded) {
        m_videoLoaded = true;
        QTimer::singleShot(60, this, [this]() {
            ui->preview->loadVideo(m_clipPath);

            double d = ui->preview->getDuration();
            if (d > 0.0 && std::fabs(d - m_duration) > 0.5) {
                m_duration = d;
                ui->progressSlider->setRange(0, toSliderVal(m_duration));
                ui->durationLabel->setText("/ " + formatTime(m_duration));
                ui->totalDurationLabel->setText("/ " + formatTime(m_duration));
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

void ClipEditDialog::onPlayPauseClicked() {
    if (!m_videoLoaded) return;
    if (ui->preview->isPlaying()) {
        ui->preview->pause();
        ui->playPauseBtn->setText("▶");
    } else {
        ui->preview->play();
        ui->playPauseBtn->setText("⏸");
    }
}

void ClipEditDialog::onProgressSliderMoved(int value) {
    double t = fromSliderVal(value);
    if (!ui->timestampEdit->hasFocus())
        ui->timestampEdit->setText(formatTime(t));
}

void ClipEditDialog::onProgressSliderReleased() {
    m_sliderDragging = false;
    double t = fromSliderVal(ui->progressSlider->value());
    seekTo(t, false);
}

void ClipEditDialog::onTimestampEditFinished() {
    bool ok = false;
    double t = parseTime(ui->timestampEdit->text(), &ok);
    if (!ok) {
        setStatus("Invalid time format — use M:SS.ss");
        ui->timestampEdit->setText(formatTime(ui->preview->getCurrentTime()));
        return;
    }
    setStatus({});
    seekTo(clampTime(t));
}

void ClipEditDialog::onSetStart() {
    double t = ui->preview->getCurrentTime();
    if (t >= m_endTime - 0.1) {
        setStatus("Start must be at least 0.1 s before end (" + formatTime(m_endTime) + ")");
        return;
    }
    m_startTime = t;
    setStatus({});
    updateSelectionLabels();
}

void ClipEditDialog::onSetEnd() {
    double t = ui->preview->getCurrentTime();
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

    ui->playPauseBtn->setText(ui->preview->isPlaying() ? "⏸" : "▶");

    double t = ui->preview->getCurrentTime();

    if (!m_sliderDragging) {
        ui->progressSlider->blockSignals(true);
        ui->progressSlider->setValue(toSliderVal(t));
        ui->progressSlider->blockSignals(false);
    }

    if (!ui->timestampEdit->hasFocus())
        ui->timestampEdit->setText(formatTime(t));
}

void ClipEditDialog::seekRelative(double delta) {
    if (!m_videoLoaded) return;
    double t = clampTime(ui->preview->getCurrentTime() + delta);
    seekTo(t);
}

void ClipEditDialog::seekTo(double secs, bool updateSlider) {
    if (!m_videoLoaded) return;
    secs = clampTime(secs);
    bool wasPlaying = ui->preview->isPlaying();
    if (wasPlaying) ui->preview->pause();
    ui->preview->seek(secs);
    if (wasPlaying) ui->preview->play();

    if (updateSlider) {
        ui->progressSlider->blockSignals(true);
        ui->progressSlider->setValue(toSliderVal(secs));
        ui->progressSlider->blockSignals(false);
    }
    if (!ui->timestampEdit->hasFocus())
        ui->timestampEdit->setText(formatTime(secs));
}

void ClipEditDialog::updateSelectionLabels() {
    ui->startTimeLabel->setText(formatTime(m_startTime));
    ui->endTimeLabel->setText(formatTime(m_endTime));
}

void ClipEditDialog::setStatus(const QString &msg, bool error) {
    ui->statusLabel->setText(msg);
    ui->statusLabel->setStyleSheet(
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
