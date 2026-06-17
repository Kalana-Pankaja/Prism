#pragma once

#include <QObject>
#include <QList>
#include <QPointer>

class VideoWidget;
class MirrorOutputWindow;

/// Routes the program compositor feed to one or more mirror output windows.
/// Future sinks (NDI, recording) attach here in later steps.
class OutputHub : public QObject {
    Q_OBJECT

public:
    explicit OutputHub(QObject *parent = nullptr);

    void setProgramSource(VideoWidget *source);
    VideoWidget *programSource() const { return m_source; }

    /// Opens a mirror window showing the current program mix.
    MirrorOutputWindow *addMirrorOutput(const QString &title = {});

    const QList<QPointer<MirrorOutputWindow>> &mirrorOutputs() const { return m_mirrors; }

private slots:
    void onProgramFrameReady();
    void onMirrorDestroyed(QObject *obj);

private:
    void placeOnSecondaryScreen(QWidget *window);

    VideoWidget *m_source = nullptr;
    QList<QPointer<MirrorOutputWindow>> m_mirrors;
};
