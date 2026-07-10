#include "core/sources/LayerCompositorSource.h"

namespace {
/// Wrap the inner source's current frame as a non-owning QImage (matching the
/// convention used by the CPU effect decorators).
QImage wrapFrame(MediaSource *src) {
    const QSize sz = src->frameSize();
    if (sz.isEmpty()) return {};
    const uint8_t *data = src->frameData();
    if (!data) return {};
    const QImage::Format fmt = src->hasAlpha() ? QImage::Format_RGBA8888
                                               : QImage::Format_RGB888;
    return QImage(data, sz.width(), sz.height(), fmt);
}
} // namespace

bool LayerCompositorSource::nextFrame() {
    if (m_canvas.isEmpty()) return false;

    // Advance every sub-layer; recomposite if anything moved or on the first frame.
    bool anyNew = (m_outTex == 0);
    bool anyReady = false;
    for (Layer &l : m_layers) {
        if (!l.source) continue;
        if (l.source->nextFrame()) anyNew = true;
        if (l.source->isReady())   anyReady = true;
    }
    if (!anyNew) return false;
    if (!anyReady) return false;

    if (!m_runner.beginComposite(m_canvas)) return false;

    for (Layer &l : m_layers) {
        if (!l.visible || !l.source || !l.source->isReady()) continue;
        unsigned int tex = l.source->glTexture();
        if (tex == 0) {
            const QImage frame = wrapFrame(l.source.get());
            if (frame.isNull()) continue;
            tex = m_runner.uploadScratch(frame.convertToFormat(QImage::Format_RGBA8888));
        }
        m_runner.drawCompositeLayer(tex, l.bx, l.by, l.bw, l.bh, l.flipH, l.flipV);
    }

    m_outTex = m_runner.lastOutput();
    m_runner.endComposite();
    m_cpuValid = false;
    return true;
}

const uint8_t *LayerCompositorSource::frameData() const {
    if (m_outTex == 0) return nullptr;
    if (!m_cpuValid) {
        m_cpu = m_runner.readback();
        m_cpuValid = true;
    }
    return m_cpu.isNull() ? nullptr
                          : reinterpret_cast<const uint8_t *>(m_cpu.constBits());
}
