#pragma once

#include <QFrame>
#include "core/OverlayItem.h"

namespace Ui { class ClipCard; }

class ClipCard : public QFrame {
    Q_OBJECT
public:
    explicit ClipCard(int index, QWidget *parent = nullptr);
    ~ClipCard();

    void loadClip(const QString &path, const QPixmap &thumbnail);
    void clearClip();
    void setActive(bool active);
    void setASelected(bool selected);
    void setBSelected(bool selected);

    QString clipPath() const { return m_clipPath; }
    bool isMuted() const { return m_muted; }
    int  volume() const;
    bool isRepeat() const { return m_repeat; }

    // Expose full settings (trim + crop + overlays)
    const ClipSettings &settings() const { return m_settings; }
    double startTime() const { return m_settings.startTime; }
    double endTime()   const { return m_settings.endTime; }
    float  cropX()     const { return m_settings.cropX; }
    float  cropY()     const { return m_settings.cropY; }
    float  cropW()     const { return m_settings.cropW; }
    float  cropH()     const { return m_settings.cropH; }
    const QList<OverlayItem> &overlays() const { return m_settings.overlays; }

signals:
    void triggered(int index);
    void aButtonClicked(int index);
    void bButtonClicked(int index);

private slots:
    void onMuteClicked();
    void onRepeatClicked();
    void onEditClicked();
    void onAButtonClicked();
    void onBButtonClicked();

private:
    Ui::ClipCard *ui;
    int           m_index;
    QString       m_clipPath;
    bool          m_muted   = false;
    bool          m_repeat  = false;
    bool          m_aSelected = false;
    bool          m_bSelected = false;
    ClipSettings  m_settings;
};
