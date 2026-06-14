#pragma once

#include <QDialog>
#include <QTimer>

namespace Ui { class ClipEditDialog; }

class ClipEditDialog : public QDialog {
    Q_OBJECT
public:
    ClipEditDialog(const QString &clipPath, double startTime, double endTime, QWidget *parent = nullptr);
    ~ClipEditDialog();

    double startTime() const { return m_startTime; }
    double endTime() const { return m_endTime; }

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onPlayPauseClicked();
    void onProgressSliderMoved(int value);
    void onProgressSliderReleased();
    void onTimestampEditFinished();
    void onSetStart();
    void onSetEnd();
    void onPollTimer();

private:
    Ui::ClipEditDialog *ui;
    QTimer *pollTimer;
    QString m_clipPath;
    double m_startTime;
    double m_endTime;
    double m_duration = 0.0;
    bool m_videoLoaded = false;
    bool m_sliderDragging = false;

    void seekRelative(double delta);
    void seekTo(double secs, bool updateSlider = true);
    void updateSelectionLabels();
    void setStatus(const QString &msg, bool error = true);
    QString formatTime(double secs) const;
    double parseTime(const QString &str, bool *ok = nullptr) const;
    double clampTime(double t) const;
    int toSliderVal(double secs) const;
    double fromSliderVal(int val) const;
};
