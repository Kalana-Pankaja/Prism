#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QElapsedTimer>
#include <QImage>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct SwsContext;
struct AVFrame;
struct AVPacket;

/// Encodes the program compositor feed to disk via FFmpeg (H.264/MKV).
class ProgramRecorder : public QObject {
    Q_OBJECT

public:
    struct Marker {
        qint64  timeMs = 0;
        QString label;
    };

    explicit ProgramRecorder(QObject *parent = nullptr);
    ~ProgramRecorder() override;

    static QString defaultOutputPath();

    bool isRecording() const { return m_recording; }
    QString outputPath() const { return m_outputPath; }
    QString markersPath() const { return m_markersPath; }
    qint64 recordingDurationMs() const;
    const QVector<Marker> &markers() const { return m_markers; }

    bool startRecording(const QString &outputPath = {});
    void stopRecording();

    void submitFrame(const QImage &frame);
    void addMarker(const QString &label);

signals:
    void recordingChanged(bool recording);
    void errorOccurred(const QString &message);

private:
    void cleanup();
    void writeMarkersFile() const;
    bool flushEncoder();

    bool     m_recording = false;
    QString  m_outputPath;
    QString  m_markersPath;
    QVector<Marker> m_markers;
    QElapsedTimer   m_timer;

    AVFormatContext *m_fmtCtx    = nullptr;
    AVCodecContext  *m_codecCtx  = nullptr;
    AVStream        *m_stream     = nullptr;
    SwsContext      *m_swsCtx     = nullptr;
    AVFrame         *m_yuvFrame   = nullptr;
    AVPacket        *m_packet     = nullptr;
    int64_t          m_frameIndex = 0;

    static constexpr int kWidth  = 1280;
    static constexpr int kHeight = 720;
    static constexpr int kFps    = 30;
};
