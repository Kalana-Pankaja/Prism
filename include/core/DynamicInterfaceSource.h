#pragma once
#include "core/MediaSource.h"
#include <QString>
#include <QSize>
#include <QByteArray>

class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFramebufferObject;
class QQuickRenderControl;
class QQuickWindow;
class QQmlEngine;
class QQmlComponent;
class QQuickItem;

class DynamicInterfaceSource : public MediaSource {
public:
    explicit DynamicInterfaceSource(const QString &qmlCode,
                                    QSize size = QSize(1280, 720));
    ~DynamicInterfaceSource() override;

    void    setQmlCode(const QString &code);
    QString qmlCode()   const { return m_qmlCode; }
    bool    hasError()  const { return !m_lastError.isEmpty(); }
    QString lastError() const { return m_lastError; }

    Type           type()      const override { return Type::DynamicInterface; }
    bool           isReady()   const override { return m_ready; }
    QSize          frameSize() const override { return m_size; }
    const uint8_t *frameData() const override {
        return reinterpret_cast<const uint8_t *>(m_buffer.constData());
    }
    bool    nextFrame()   override;
    QString displayName() const override { return "Dynamic Interface"; }

private:
    bool initQml();
    void destroyQml();
    void reloadComponent();
    void renderToBuffer();

    QString    m_qmlCode;
    QSize      m_size;
    QByteArray m_buffer;

    QOffscreenSurface        *m_surface       = nullptr;
    QOpenGLContext           *m_glContext      = nullptr;
    QOpenGLFramebufferObject *m_fbo            = nullptr;
    QQuickRenderControl      *m_renderControl  = nullptr;
    QQuickWindow             *m_quickWindow    = nullptr;
    QQmlEngine               *m_engine         = nullptr;
    QQmlComponent            *m_component      = nullptr;
    QQuickItem               *m_rootItem       = nullptr;

    bool    m_initialized = false;
    bool    m_ready       = false;
    bool    m_qmlDirty    = false;
    QString m_lastError;
};
