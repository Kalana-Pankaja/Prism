#include "core/sources/CameraSource.h"
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QMediaDevices>
#include <QApplication>
#include <QMessageBox>
#include <QDebug>

namespace {

QString friendlyCameraError(QCamera::Error err, const QString &msg) {
    Q_UNUSED(err);
    if (msg.contains(QStringLiteral("general stream error"), Qt::CaseInsensitive)
        || msg.contains(QStringLiteral("GStreamer"), Qt::CaseInsensitive)) {
        return QObject::tr(
            "The camera could not start.\n\n"
            "Common causes:\n"
            "• Another app is using the camera (OBS, browser, Zoom, etc.)\n"
            "• The selected device path is wrong — try \"Default Camera\"\n"
            "• The camera driver needs a moment after unplug/replug\n\n"
            "Technical detail: %1").arg(msg);
    }
    return msg.isEmpty()
        ? QObject::tr("The camera could not start.")
        : msg;
}

}

CameraSource::CameraSource() = default;

CameraSource::~CameraSource() {
    stop();
}

bool CameraSource::start(const QCameraDevice &device) {
    stop();

    // If a valid device was provided, use it; otherwise let Qt pick the default.
    // Deliberately do NOT call QMediaDevices::defaultVideoInput() here — on some
    // hardware (Intel IPU6 / MIPI cameras) QMediaDevices::videoInputs() returns
    // empty even though QCamera() with no arguments can still access the camera.
    if (!device.isNull()) {
        m_name   = device.description().isEmpty()
                 ? QString::fromUtf8(device.id())
                 : device.description();
        m_camera = new QCamera(device);
    } else {
        m_name   = "Default Camera";
        m_camera = new QCamera();   // Qt chooses the default camera internally
    }

    m_session = new QMediaCaptureSession();
    m_sink    = new QVideoSink();

    // Do NOT call setCameraFormat() — on Intel IPU6 / MIPI cameras forcing a
    // format causes "poll error: Invalid argument". Let GStreamer auto-negotiate.

    m_session->setCamera(m_camera);
    m_session->setVideoSink(m_sink);

    connect(m_sink, &QVideoSink::videoFrameChanged,
            this,   &CameraSource::onVideoFrameChanged,
            Qt::QueuedConnection);

    connect(m_camera, &QCamera::errorOccurred, this, [this](QCamera::Error err, const QString &msg) {
        m_lastError = friendlyCameraError(err, msg);
        m_failed = true;
        qWarning() << "CameraSource error:" << err << msg;
        if (QWidget *w = QApplication::activeWindow()) {
            QMessageBox::warning(w, tr("Camera Error"), m_lastError);
        }
        stop();
    });

    m_failed = false;
    m_lastError.clear();
    m_camera->start();
    qDebug() << "CameraSource: started" << m_name;
    return true;  // starting is async — success is confirmed when frames arrive
}

bool CameraSource::startDevice(const QString &devicePath) {
    const auto all = QMediaDevices::videoInputs();
    for (const auto &dev : all) {
        const QString id = QString::fromUtf8(dev.id());
        if (id == devicePath)
            return start(dev);
#ifdef Q_OS_WIN
        // Media Foundation symbolic links may appear with or without a @device: prefix.
        if (!devicePath.isEmpty()) {
            const QString normalized = id.startsWith(QStringLiteral("@device:"))
                ? id.mid(8) : id;
            const QString pathNorm = devicePath.startsWith(QStringLiteral("@device:"))
                ? devicePath.mid(8) : devicePath;
            if (normalized.compare(pathNorm, Qt::CaseInsensitive) == 0)
                return start(dev);
        }
#endif
    }
    // No Qt device matched the path — fall back to QCamera() with no device
    // argument (system default).  This works even for Intel IPU6 / MIPI cameras
    // where QMediaDevices::videoInputs() returns empty.
    qDebug() << "CameraSource: path" << devicePath
             << "not in Qt device list — using QCamera() default";
    m_name = devicePath.isEmpty() ? "Default Camera" : devicePath;
    return start({});   // {} = null QCameraDevice → QCamera() with no device
}

void CameraSource::stop() {
    if (m_camera)  m_camera->stop();
    if (m_session) {
        m_session->setCamera(nullptr);
        m_session->setVideoSink(nullptr);
    }
    delete m_sink;    m_sink    = nullptr;
    delete m_session; m_session = nullptr;
    delete m_camera;  m_camera  = nullptr;
    m_frame = {};
    m_dirty = false;
    m_failed = false;
}

void CameraSource::onVideoFrameChanged(const QVideoFrame &frame) {
    if (!frame.isValid()) return;
    QImage img = frame.toImage().convertToFormat(QImage::Format_RGB888);
    if (!img.isNull()) {
        m_frame = std::move(img);
        m_dirty = true;
    }
}

bool CameraSource::nextFrame() {
    if (!m_dirty) return false;
    m_dirty = false;
    return true;
}
