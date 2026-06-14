#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <memory>
#include "../core/VideoPlayer.h"

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    void loadVideo(const QString &filePath);
    void play();
    void pause();
    void stop();
    void seek(double seconds);

    void setRepeat(bool repeat) { m_repeat = repeat; }
    void setTrimPoints(double startSec, double endSec);

    bool isPlaying() const { return playing; }
    double getCurrentTime() const;
    double getDuration() const;

    QSize sizeHint() const override { return QSize(1280, 720); }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void updateFrame();

private:
    std::unique_ptr<VideoPlayer> player;
    GLuint texture = 0;
    bool playing = false;
    bool m_repeat = false;
    double m_trimStart = 0.0;
    double m_trimEnd = -1.0;
    QTimer *frameTimer = nullptr;

    void setupTexture();
    void updateTexture();
};
