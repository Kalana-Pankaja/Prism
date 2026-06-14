#pragma once

#include <QFrame>

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
    int volume() const;
    bool isRepeat() const { return m_repeat; }
    double startTime() const { return m_startTime; }
    double endTime() const { return m_endTime; }

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
    int m_index;
    QString m_clipPath;
    bool m_muted = false;
    bool m_repeat = false;
    double m_startTime = 0.0;
    double m_endTime = -1.0;
    bool m_aSelected = false;
    bool m_bSelected = false;
};
