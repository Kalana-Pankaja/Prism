#include "core/DynamicInterfaceSource.h"
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickRenderTarget>
#include <QSGRendererInterface>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QCoreApplication>
#include <QDebug>


DynamicInterfaceSource::DynamicInterfaceSource(const QString &qmlCode, QSize size)
    : m_qmlCode(qmlCode), m_size(size)
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
}

DynamicInterfaceSource::~DynamicInterfaceSource() {
    destroyQml();
}

void DynamicInterfaceSource::setQmlCode(const QString &code) {
    m_qmlCode  = code;
    m_qmlDirty = true;
    m_ready    = false;
}

bool DynamicInterfaceSource::nextFrame() {
    if (!m_initialized && !initQml())
        return false;

    if (m_qmlDirty) {
        m_glContext->makeCurrent(m_surface);
        reloadComponent();
        m_glContext->doneCurrent();
        m_qmlDirty = false;
    }

    if (!m_ready || !m_rootItem)
        return false;

    m_glContext->makeCurrent(m_surface);
    renderToBuffer();
    m_glContext->doneCurrent();
    return true;
}

bool DynamicInterfaceSource::initQml() {
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);

    m_surface = new QOffscreenSurface();
    m_surface->setFormat(fmt);
    m_surface->create();
    if (!m_surface->isValid()) {
        m_lastError = "Failed to create offscreen surface";
        delete m_surface; m_surface = nullptr;
        return false;
    }

    m_glContext = new QOpenGLContext();
    m_glContext->setFormat(fmt);
    if (!m_glContext->create()) {
        m_lastError = "Failed to create OpenGL context";
        delete m_surface;   m_surface   = nullptr;
        delete m_glContext; m_glContext = nullptr;
        return false;
    }

    m_glContext->makeCurrent(m_surface);

    QOpenGLFramebufferObjectFormat fboFmt;
    fboFmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    m_fbo = new QOpenGLFramebufferObject(m_size, fboFmt);
    if (!m_fbo->isValid()) {
        m_lastError = "Failed to create FBO";
        m_glContext->doneCurrent();
        delete m_fbo;       m_fbo       = nullptr;
        delete m_glContext; m_glContext = nullptr;
        delete m_surface;   m_surface   = nullptr;
        return false;
    }

    m_renderControl = new QQuickRenderControl();
    m_quickWindow   = new QQuickWindow(m_renderControl);
    m_quickWindow->setGeometry(0, 0, m_size.width(), m_size.height());
    m_quickWindow->setColor(Qt::transparent);
    m_quickWindow->setRenderTarget(
        QQuickRenderTarget::fromOpenGLTexture(m_fbo->texture(), m_size));
    m_renderControl->initialize();

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    if (!m_renderControl->rhi()) {
        m_lastError = "RHI initialization failed — OpenGL backend unavailable";
        m_glContext->doneCurrent();
        delete m_quickWindow;   m_quickWindow   = nullptr;
        delete m_renderControl; m_renderControl = nullptr;
        delete m_fbo;           m_fbo           = nullptr;
        delete m_glContext;     m_glContext     = nullptr;
        delete m_surface;       m_surface       = nullptr;
        return false;
    }
#endif

    m_engine      = new QQmlEngine();
    m_initialized = true;

    reloadComponent();

    m_glContext->doneCurrent();
    return m_ready;
}

void DynamicInterfaceSource::reloadComponent() {
    // Caller must have GL context current.
    if (m_rootItem) {
        m_rootItem->setParentItem(nullptr);
        delete m_rootItem;
        m_rootItem = nullptr;
    }
    delete m_component;
    m_component = nullptr;
    m_ready     = false;

    m_component = new QQmlComponent(m_engine);
    m_component->setData(m_qmlCode.toUtf8(), QUrl());

    if (m_component->isError()) {
        m_lastError = m_component->errorString();
        m_buffer.fill(0, m_size.width() * m_size.height() * 3);
        return;
    }

    QObject *obj = m_component->create();
    if (!obj || m_component->isError()) {
        m_lastError = m_component->errorString();
        m_buffer.fill(0, m_size.width() * m_size.height() * 3);
        delete obj;
        return;
    }

    m_rootItem = qobject_cast<QQuickItem *>(obj);
    if (!m_rootItem) {
        m_lastError = "Root QML object is not a visual item";
        m_buffer.fill(0, m_size.width() * m_size.height() * 3);
        delete obj;
        return;
    }

    m_rootItem->setParentItem(m_quickWindow->contentItem());
    m_rootItem->setWidth(m_size.width());
    m_rootItem->setHeight(m_size.height());
    m_lastError.clear();

    // Release context while the event loop processes QML scene graph init.
    m_glContext->doneCurrent();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    m_glContext->makeCurrent(m_surface);

    renderToBuffer();
    m_ready = true;
}

void DynamicInterfaceSource::destroyQml() {
    if (!m_initialized) return;

    if (m_rootItem) {
        m_rootItem->setParentItem(nullptr);
        delete m_rootItem;
        m_rootItem = nullptr;
    }
    delete m_component;    m_component    = nullptr;
    delete m_engine;       m_engine       = nullptr;

    m_glContext->makeCurrent(m_surface);
    delete m_quickWindow;   m_quickWindow  = nullptr;
    delete m_renderControl; m_renderControl = nullptr;
    delete m_fbo;           m_fbo          = nullptr;
    m_glContext->doneCurrent();

    delete m_glContext;    m_glContext    = nullptr;
    delete m_surface;      m_surface      = nullptr;
    m_initialized = false;
    m_ready       = false;
}

void DynamicInterfaceSource::renderToBuffer() {
    m_renderControl->polishItems();
    m_renderControl->beginFrame();
    m_renderControl->sync();
    m_renderControl->render();
    m_renderControl->endFrame();

    m_glContext->functions()->glFinish();

    m_fbo->bind();
    const int w = m_size.width(), h = m_size.height();
    QByteArray rgba(w * h * 4, 0);
    m_glContext->functions()->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    m_glContext->functions()->glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    m_fbo->release();

    m_buffer.resize(w * h * 3);
    auto       *dst = reinterpret_cast<uint8_t *>(m_buffer.data());
    const auto *src = reinterpret_cast<const uint8_t *>(rgba.constData());
    for (int y = 0; y < h; ++y) {
        const uint8_t *srcRow = src + (h - 1 - y) * w * 4;
        uint8_t       *dstRow = dst + y * w * 3;
        for (int x = 0; x < w; ++x) {
            dstRow[x*3    ] = srcRow[x*4    ];
            dstRow[x*3 + 1] = srcRow[x*4 + 1];
            dstRow[x*3 + 2] = srcRow[x*4 + 2];
        }
    }
}
