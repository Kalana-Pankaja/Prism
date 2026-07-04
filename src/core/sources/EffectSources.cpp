#include "core/sources/EffectSources.h"

#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace {

/// Wrap the inner source's current frame as a non-owning QImage.
QImage wrapInner(MediaSource *inner) {
    const QSize sz = inner->frameSize();
    if (sz.isEmpty()) return {};
    const uint8_t *data = inner->frameData();
    if (!data) return {};
    const QImage::Format fmt = inner->hasAlpha() ? QImage::Format_RGBA8888
                                                 : QImage::Format_RGB888;
    return QImage(data, sz.width(), sz.height(), fmt);
}

} // namespace

bool FrameEffectSource::nextFrame() {
    if (!m_inner) return false;

    const bool innerNew = m_inner->nextFrame();
    if (!innerNew && !m_output.isNull()) return false;
    if (!m_inner->isReady()) return false;

    const QImage wrapped = wrapInner(m_inner.get());
    if (wrapped.isNull()) return false;

    // Own an RGBA8888 copy so process() never aliases the inner buffer.
    const QImage rgba = wrapped.convertToFormat(QImage::Format_RGBA8888);
    m_output = process(rgba);
    return !m_output.isNull();
}

// ── Rotate ──────────────────────────────────────────────────────────────────
QImage RotateSource::process(const QImage &in) {
    if (m_angle == 0) return in;   // already an owned RGBA8888 copy
    QTransform t;
    t.rotate(m_angle);
    QImage out = in.transformed(t, Qt::FastTransformation);
    return out.convertToFormat(QImage::Format_RGBA8888);
}

// ── Keystone / perspective ────────────────────────────────────────────────────
QImage KeystoneSource::process(const QImage &in) {
    if (m_corners.size() != 4) return in;
    const qreal w = in.width();
    const qreal h = in.height();

    const QPolygonF srcQuad{{0, 0}, {w, 0}, {w, h}, {0, h}};
    const QPolygonF dstQuad{
        {m_corners[0].x() * w, m_corners[0].y() * h},
        {m_corners[1].x() * w, m_corners[1].y() * h},
        {m_corners[2].x() * w, m_corners[2].y() * h},
        {m_corners[3].x() * w, m_corners[3].y() * h}};

    QTransform t;
    if (!QTransform::quadToQuad(srcQuad, dstQuad, t))
        return in;

    QImage out(in.size(), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setTransform(t);
    p.drawImage(0, 0, in);
    p.end();
    return out.convertToFormat(QImage::Format_RGBA8888);
}

// ── Chroma key ────────────────────────────────────────────────────────────────
QImage ChromaKeySource::process(const QImage &in) {
    QImage out = in;   // owned RGBA8888
    const int w = out.width();
    const int h = out.height();

    const double kr = m_key.red();
    const double kg = m_key.green();
    const double kb = m_key.blue();
    const double maxDist = 441.6729559300637;   // sqrt(3 * 255^2)
    const double thr  = std::clamp(m_threshold, 0.0, 1.0) * maxDist;
    const double soft = std::max(0.0, m_smoothness) * maxDist;

    for (int y = 0; y < h; ++y) {
        uchar *row = out.scanLine(y);
        for (int x = 0; x < w; ++x) {
            uchar *px = row + x * 4;
            const double dr = px[0] - kr;
            const double dg = px[1] - kg;
            const double db = px[2] - kb;
            const double dist = std::sqrt(dr * dr + dg * dg + db * db);

            double keep;   // 1 = keep pixel, 0 = fully keyed out
            if (dist <= thr)
                keep = 0.0;
            else if (soft > 0.0 && dist < thr + soft)
                keep = (dist - thr) / soft;
            else
                keep = 1.0;

            px[3] = static_cast<uchar>(px[3] * keep);
        }
    }
    return out;
}

// ── Blur ──────────────────────────────────────────────────────────────────────
QImage BlurSource::process(const QImage &in) {
    const int radius = std::clamp(m_radius, 0, 64);
    if (radius <= 0) return in;

    const int w = in.width();
    const int h = in.height();
    if (w == 0 || h == 0) return in;

    std::vector<uchar> src(static_cast<size_t>(w) * h * 4);
    std::memcpy(src.data(), in.constBits(), src.size());
    std::vector<uchar> tmp(src.size());

    const int win = radius * 2 + 1;

    auto boxPass = [&](std::vector<uchar> &s, std::vector<uchar> &d, bool horizontal) {
        const int outer = horizontal ? h : w;
        const int inner = horizontal ? w : h;
        const int stepInner = horizontal ? 4 : w * 4;
        const int stepOuter = horizontal ? w * 4 : 4;
        for (int o = 0; o < outer; ++o) {
            const uchar *base = s.data() + static_cast<size_t>(o) * stepOuter;
            uchar *dbase = d.data() + static_cast<size_t>(o) * stepOuter;
            for (int c = 0; c < 4; ++c) {
                int sum = 0;
                for (int k = -radius; k <= radius; ++k) {
                    const int idx = std::clamp(k, 0, inner - 1);
                    sum += base[idx * stepInner + c];
                }
                for (int i = 0; i < inner; ++i) {
                    dbase[i * stepInner + c] = static_cast<uchar>(sum / win);
                    const int addIdx = std::clamp(i + radius + 1, 0, inner - 1);
                    const int subIdx = std::clamp(i - radius, 0, inner - 1);
                    sum += base[addIdx * stepInner + c];
                    sum -= base[subIdx * stepInner + c];
                }
            }
        }
    };

    // Two passes (H then V), repeated twice to approach a Gaussian falloff.
    for (int pass = 0; pass < 2; ++pass) {
        boxPass(src, tmp, true);
        boxPass(tmp, src, false);
    }

    QImage out(w, h, QImage::Format_RGBA8888);
    std::memcpy(out.bits(), src.data(), src.size());
    return out;
}

// ── Sharpen ────────────────────────────────────────────────────────────────────
QImage SharpenSource::process(const QImage &in) {
    const double amount = std::max(0.0, m_amount);
    if (amount <= 0.0) return in;

    const int w = in.width();
    const int h = in.height();
    if (w < 3 || h < 3) return in;

    QImage out(w, h, QImage::Format_RGBA8888);
    const double center = 1.0 + 4.0 * amount;
    const double side   = -amount;

    for (int y = 0; y < h; ++y) {
        const uchar *rowM = in.constScanLine(y);
        const uchar *rowU = in.constScanLine(std::max(0, y - 1));
        const uchar *rowD = in.constScanLine(std::min(h - 1, y + 1));
        uchar *rowO = out.scanLine(y);
        for (int x = 0; x < w; ++x) {
            const int xl = std::max(0, x - 1);
            const int xr = std::min(w - 1, x + 1);
            for (int c = 0; c < 3; ++c) {
                const double v = center * rowM[x * 4 + c]
                               + side * rowM[xl * 4 + c]
                               + side * rowM[xr * 4 + c]
                               + side * rowU[x * 4 + c]
                               + side * rowD[x * 4 + c];
                rowO[x * 4 + c] = static_cast<uchar>(std::clamp(v, 0.0, 255.0));
            }
            rowO[x * 4 + 3] = rowM[x * 4 + 3];   // preserve alpha
        }
    }
    return out;
}

// ── Polygonal masking ─────────────────────────────────────────────────────────
QImage PolygonMaskSource::process(const QImage &in) {
    if (m_points.size() < 3) return in;

    const qreal w = in.width();
    const qreal h = in.height();

    QPolygonF poly;
    poly.reserve(m_points.size());
    for (const QPointF &pt : m_points)
        poly << QPointF(pt.x() * w, pt.y() * h);

    QPainterPath path;
    path.addPolygon(poly);
    path.closeSubpath();
    if (m_invert) {
        QPainterPath full;
        full.addRect(0, 0, w, h);
        path = full.subtracted(path);
    }

    QImage out(in.size(), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipPath(path);
    p.drawImage(0, 0, in);
    p.end();
    return out.convertToFormat(QImage::Format_RGBA8888);
}

// ── Opacity ────────────────────────────────────────────────────────────────────
QImage OpacitySource::process(const QImage &in) {
    const double opacity = std::clamp(m_opacity, 0.0, 1.0);
    if (opacity >= 1.0) return in;

    QImage out = in;
    const int w = out.width();
    const int h = out.height();
    for (int y = 0; y < h; ++y) {
        uchar *row = out.scanLine(y);
        for (int x = 0; x < w; ++x)
            row[x * 4 + 3] = static_cast<uchar>(row[x * 4 + 3] * opacity);
    }
    return out;
}
