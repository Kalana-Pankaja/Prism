#pragma once

#include "core/sources/MediaSource.h"
#include "core/effects/EffectPassRunner.h"

#include <QImage>
#include <QSize>
#include <memory>
#include <vector>

/// Flattens a Layer node's inputs into one GPU-resident frame.
///
/// Each sub-layer is drawn (bottom→top, source-over alpha) into a single shared-
/// context FBO at its normalised canvas placement, and the result is exposed via
/// glTexture() so a downstream process effect — and ultimately VideoWidget's
/// compositor — operates on the flattened composite rather than on each layer
/// independently. frameData() falls back to a lazy GPU→CPU read for non-GL
/// consumers (thumbnails) and CPU effects chained after it.
class LayerCompositorSource : public MediaSource {
public:
    struct Layer {
        std::unique_ptr<MediaSource> source;
        float bx = 0.f, by = 0.f, bw = 1.f, bh = 1.f;  // normalised canvas placement
        bool  visible = true;
        bool  flipH = false, flipV = false;
    };

    LayerCompositorSource(std::vector<Layer> layers, QSize canvas)
        : m_layers(std::move(layers)), m_canvas(canvas) {}

    Type    type()        const override { return Type::Canvas; }
    bool    isReady()     const override { return m_outTex != 0; }
    QSize   frameSize()   const override { return m_canvas; }
    const uint8_t *frameData() const override;
    bool    nextFrame()         override;
    bool    hasAlpha()    const override { return true; }
    unsigned int glTexture() const override { return m_outTex; }

    void play()  override { for (auto &l : m_layers) if (l.source) l.source->play(); }
    void pause() override { for (auto &l : m_layers) if (l.source) l.source->pause(); }

private:
    std::vector<Layer> m_layers;
    QSize              m_canvas;
    mutable EffectPassRunner m_runner;   // readback() in const frameData()
    unsigned int       m_outTex = 0;
    mutable QImage     m_cpu;
    mutable bool       m_cpuValid = false;
};
