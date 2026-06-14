#include "ui/VideoWidget.h"
#include <QTimer>
#include <QDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QPainter>
#include <QFont>
#include <QFileInfo>
#include <algorithm>

VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_playerA(std::make_unique<VideoPlayer>())
    , m_playerB(std::make_unique<VideoPlayer>()) {
    setAcceptDrops(true);
    setStyleSheet("background-color: #000;");

    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &VideoWidget::updateFrame);
    m_frameTimer->start(33); // ~30 FPS, always running
}

VideoWidget::~VideoWidget() {
    m_frameTimer->stop();
    makeCurrent();
    if (m_textureA) glDeleteTextures(1, &m_textureA);
    if (m_textureB) glDeleteTextures(1, &m_textureB);
    doneCurrent();
}

// ── OpenGL callbacks ──────────────────────────────────────────────────────────

void VideoWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
}

void VideoWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void VideoWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLuint tex = m_showA ? m_textureA : m_textureB;
    VideoPlayer *player = m_showA ? m_playerA.get() : m_playerB.get();
    if (!tex || !player->isOpen()) return;

    float cx, cy, cw, ch;
    if (m_showA) { cx=m_cropXA; cy=m_cropYA; cw=m_cropWA; ch=m_cropHA; }
    else         { cx=m_cropXB; cy=m_cropYB; cw=m_cropWB; ch=m_cropHB; }

    // Compute letterboxed rect and cache it for overlay rendering
    m_videoRect = computeVideoRect(player, cx, cy, cw, ch);
    renderTexture(tex, cx, cy, cw, ch,
                  (float)m_videoRect.x(),     (float)m_videoRect.y(),
                  (float)m_videoRect.width(), (float)m_videoRect.height());
}

void VideoWidget::paintEvent(QPaintEvent *e) {
    QOpenGLWidget::paintEvent(e);  // runs initializeGL / resizeGL / paintGL

    const auto &overlays = m_showA ? m_overlaysA : m_overlaysB;
    if (overlays.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    renderOverlays(p, overlays);
}

void VideoWidget::renderTexture(GLuint tex, float cx, float cy, float cw, float ch,
                                float dstX, float dstY, float dstW, float dstH) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_QUADS);
    glTexCoord2f(cx,      cy);      glVertex2f(dstX,        dstY);
    glTexCoord2f(cx + cw, cy);      glVertex2f(dstX + dstW, dstY);
    glTexCoord2f(cx + cw, cy + ch); glVertex2f(dstX + dstW, dstY + dstH);
    glTexCoord2f(cx,      cy + ch); glVertex2f(dstX,        dstY + dstH);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Returns the letterboxed rect (in widget pixels) for the given video/crop combo.
QRectF VideoWidget::computeVideoRect(VideoPlayer *player,
                                     float cx, float cy, float cw, float ch) const {
    Q_UNUSED(cx); Q_UNUSED(cy);
    QSize fs = player->getFrameSize();
    if (fs.isEmpty()) return QRectF(0, 0, width(), height());

    // Aspect ratio of the *visible* (cropped) portion
    float videoAR = (fs.width() * cw) / (fs.height() * ch);
    float widgetAR = (float)width() / (float)height();

    float dstW, dstH, dstX, dstY;
    if (videoAR > widgetAR) {           // wider: pillarbox (bars top/bottom)
        dstW = (float)width();
        dstH = dstW / videoAR;
        dstX = 0.f;
        dstY = ((float)height() - dstH) / 2.f;
    } else {                            // taller: letterbox (bars left/right)
        dstH = (float)height();
        dstW = dstH * videoAR;
        dstX = ((float)width() - dstW) / 2.f;
        dstY = 0.f;
    }
    return QRectF(dstX, dstY, dstW, dstH);
}

void VideoWidget::renderOverlays(QPainter &p, const QList<OverlayItem> &overlays) {
    // Position overlays relative to the video display rect, not the whole widget.
    const QRectF &vr = m_videoRect.isEmpty()
                     ? QRectF(0, 0, width(), height())
                     : m_videoRect;

    for (const auto &ov : overlays) {
        if (!ov.visible) continue;
        QRectF r(vr.left() + ov.x * vr.width(),
                 vr.top()  + ov.y * vr.height(),
                 ov.w * vr.width(),
                 ov.h * vr.height());
        p.setOpacity(static_cast<double>(ov.opacity));
        if (ov.type == OverlayItem::Type::Text) {
            QFont f;
            f.setPixelSize(std::max(8, static_cast<int>(ov.fontSize * vr.width() / 1280.0)));
            p.setFont(f);
            p.setPen(ov.color);
            p.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, ov.content);
        } else {
            if (!m_overlayPixCache.contains(ov.content))
                m_overlayPixCache.insert(ov.content, QPixmap(ov.content));
            const QPixmap &pm = m_overlayPixCache[ov.content];
            if (!pm.isNull())
                p.drawPixmap(r.toRect(),
                    pm.scaled(r.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    p.setOpacity(1.0);
}

// ── Load ─────────────────────────────────────────────────────────────────────

void VideoWidget::loadVideoA(const QString &filePath) {
    m_playingA = false;
    if (!m_playerA->open(filePath)) return;
    makeCurrent();
    setupTextureGL(m_textureA, m_playerA.get());
    m_playerA->decodeFrame(); // prime first frame for preview
    uploadFrameGL(m_textureA, m_playerA.get());
    doneCurrent();
    if (m_showA) update();
}

void VideoWidget::loadVideoB(const QString &filePath) {
    m_playingB = false;
    if (!m_playerB->open(filePath)) return;
    makeCurrent();
    setupTextureGL(m_textureB, m_playerB.get());
    m_playerB->decodeFrame();
    uploadFrameGL(m_textureB, m_playerB.get());
    doneCurrent();
    if (!m_showA) update();
}

// ── Playback control ──────────────────────────────────────────────────────────

void VideoWidget::playA()  { if (m_playerA->isOpen()) m_playingA = true; }
void VideoWidget::pauseA() { m_playingA = false; }
void VideoWidget::playB()  { if (m_playerB->isOpen()) m_playingB = true; }
void VideoWidget::pauseB() { m_playingB = false; }

void VideoWidget::stop() {
    m_playingA = false;
    m_playingB = false;
}

void VideoWidget::seekA(double seconds) {
    if (!m_playerA->isOpen()) return;
    m_playerA->seek(seconds);
    if (m_playerA->decodeFrame()) {
        makeCurrent();
        uploadFrameGL(m_textureA, m_playerA.get());
        doneCurrent();
        if (m_showA) update();
    }
}

void VideoWidget::seekB(double seconds) {
    if (!m_playerB->isOpen()) return;
    m_playerB->seek(seconds);
    if (m_playerB->decodeFrame()) {
        makeCurrent();
        uploadFrameGL(m_textureB, m_playerB.get());
        doneCurrent();
        if (!m_showA) update();
    }
}

// ── Trim ─────────────────────────────────────────────────────────────────────

void VideoWidget::setTrimPointsA(double s, double e) { m_trimStartA = s; m_trimEndA = e; }
void VideoWidget::setTrimPointsB(double s, double e) { m_trimStartB = s; m_trimEndB = e; }

// ── Crop ─────────────────────────────────────────────────────────────────────

void VideoWidget::setCropA(float x, float y, float w, float h) {
    m_cropXA = x; m_cropYA = y; m_cropWA = w; m_cropHA = h;
    if (m_showA) update();
}

void VideoWidget::setCropB(float x, float y, float w, float h) {
    m_cropXB = x; m_cropYB = y; m_cropWB = w; m_cropHB = h;
    if (!m_showA) update();
}

// ── Overlays ─────────────────────────────────────────────────────────────────

void VideoWidget::setOverlaysA(const QList<OverlayItem> &overlays) {
    m_overlaysA = overlays;
    m_overlayPixCache.clear();
    update();
}

void VideoWidget::setOverlaysB(const QList<OverlayItem> &overlays) {
    m_overlaysB = overlays;
    m_overlayPixCache.clear();
    update();
}

// ── Query ─────────────────────────────────────────────────────────────────────

double VideoWidget::getCurrentTimeA() const { return m_playerA->isOpen() ? m_playerA->getCurrentTime() : 0.0; }
double VideoWidget::getDurationA()    const { return m_playerA->isOpen() ? m_playerA->getDuration()    : 0.0; }
double VideoWidget::getCurrentTimeB() const { return m_playerB->isOpen() ? m_playerB->getCurrentTime() : 0.0; }
double VideoWidget::getDurationB()    const { return m_playerB->isOpen() ? m_playerB->getDuration()    : 0.0; }

void VideoWidget::setShowA(bool showA) {
    m_showA = showA;
    update();
}

QImage VideoWidget::getFrameA() const {
    if (!m_playerA->isOpen()) return {};
    const uint8_t *data = m_playerA->getFrameData();
    if (!data) return {};
    QSize sz = m_playerA->getFrameSize();
    return QImage(data, sz.width(), sz.height(), sz.width() * 3, QImage::Format_RGB888).copy();
}

QImage VideoWidget::getFrameB() const {
    if (!m_playerB->isOpen()) return {};
    const uint8_t *data = m_playerB->getFrameData();
    if (!data) return {};
    QSize sz = m_playerB->getFrameSize();
    return QImage(data, sz.width(), sz.height(), sz.width() * 3, QImage::Format_RGB888).copy();
}

// ── GL helpers ────────────────────────────────────────────────────────────────

void VideoWidget::setupTextureGL(GLuint &tex, VideoPlayer *player) {
    if (tex) { glDeleteTextures(1, &tex); tex = 0; }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    QSize sz = player->getFrameSize();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sz.width(), sz.height(),
                 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoWidget::uploadFrameGL(GLuint tex, VideoPlayer *player) {
    if (!tex || !player->isOpen()) return;
    const uint8_t *data = player->getFrameData();
    if (!data) return;
    QSize sz = player->getFrameSize();
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sz.width(), sz.height(),
                    GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ── Frame loop ────────────────────────────────────────────────────────────────

bool VideoWidget::advancePlayer(VideoPlayer *player, bool &playing, bool repeat,
                                double trimStart, double trimEnd) {
    if (!player->isOpen()) return false;

    double endLimit = (trimEnd > 0) ? trimEnd : player->getDuration();
    if (endLimit > 0 && player->getCurrentTime() >= endLimit) {
        if (repeat) {
            player->seek(trimStart);
        } else {
            playing = false;
            return false;
        }
    }

    return player->decodeFrame();
}

void VideoWidget::updateFrame() {
    if (!m_playingA && !m_playingB) return;

    bool decodedA = false, decodedB = false;

    if (m_playingA)
        decodedA = advancePlayer(m_playerA.get(), m_playingA, m_repeatA, m_trimStartA, m_trimEndA);
    if (m_playingB)
        decodedB = advancePlayer(m_playerB.get(), m_playingB, m_repeatB, m_trimStartB, m_trimEndB);

    if (decodedA || decodedB) {
        makeCurrent();
        if (decodedA) uploadFrameGL(m_textureA, m_playerA.get());
        if (decodedB) uploadFrameGL(m_textureB, m_playerB.get());
        doneCurrent();

        if ((m_showA && decodedA) || (!m_showA && decodedB))
            update();
    }
}

// ── Drag & drop ───────────────────────────────────────────────────────────────

void VideoWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void VideoWidget::dropEvent(QDropEvent *event) {
    const QMimeData *md = event->mimeData();
    if (md->hasUrls()) {
        loadVideoA(md->urls().first().toLocalFile());
        event->acceptProposedAction();
    }
}
