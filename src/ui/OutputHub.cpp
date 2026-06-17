#include "ui/OutputHub.h"
#include "ui/MirrorOutputWindow.h"
#include "ui/VideoWidget.h"
#include <QGuiApplication>
#include <QScreen>

OutputHub::OutputHub(QObject *parent)
    : QObject(parent)
{
}

void OutputHub::setProgramSource(VideoWidget *source) {
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = source;
    if (!m_source) return;

    connect(m_source, &VideoWidget::programFrameReady,
            this, &OutputHub::onProgramFrameReady);
}

MirrorOutputWindow *OutputHub::addMirrorOutput(const QString &title) {
    if (!m_source) return nullptr;

    auto *window = new MirrorOutputWindow();
    window->setAttribute(Qt::WA_DeleteOnClose);

    if (!title.isEmpty())
        window->setWindowTitle(title);

    connect(window, &QObject::destroyed, this, &OutputHub::onMirrorDestroyed);

    m_mirrors.append(window);
    m_source->addProgramFrameConsumer();
    placeOnSecondaryScreen(window);
    window->show();
    return window;
}

void OutputHub::onProgramFrameReady() {
    if (!m_source) return;
    const QImage frame = m_source->programFrame();
    if (frame.isNull()) return;

    for (const auto &mirror : m_mirrors) {
        if (mirror)
            mirror->setFrame(frame);
    }
}

void OutputHub::onMirrorDestroyed(QObject *obj) {
    m_mirrors.removeAll(static_cast<MirrorOutputWindow *>(obj));
    if (m_source)
        m_source->removeProgramFrameConsumer();
}

void OutputHub::placeOnSecondaryScreen(QWidget *window) {
    const auto screens = QGuiApplication::screens();
    if (screens.size() < 2) return;

    QScreen *secondary = screens.at(1);
    window->setScreen(secondary);
    const QRect geo = secondary->availableGeometry();
    window->move(geo.topLeft() + QPoint(40, 40));
}
