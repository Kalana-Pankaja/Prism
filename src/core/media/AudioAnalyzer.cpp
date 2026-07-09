#include "core/media/AudioAnalyzer.h"
#include "core/media/AudioDecoder.h"

#include "kiss_fftr.h"

#include <algorithm>
#include <cmath>

static constexpr int kSampleRate = AudioDecoder::kOutputSampleRate;

AudioAnalyzer::AudioAnalyzer()
    : m_decoder(new AudioDecoder())
    , m_ringBuffer(kFftSize, 0.f)
    , m_hannWindow(kFftSize)
    , m_fftInput(kFftSize)
    , m_spectrum(kBins, 0.f)
    , m_smoothedSpectrum(kBins, 0.f)
{
    for (int i = 0; i < kFftSize; ++i) {
        const float w = static_cast<float>(i) / static_cast<float>(kFftSize - 1);
        m_hannWindow[i] = 0.5f * (1.f - std::cos(2.f * static_cast<float>(M_PI) * w));
    }
}

AudioAnalyzer::~AudioAnalyzer() {
    if (m_fftCfg) {
        kiss_fftr_free(static_cast<kiss_fftr_cfg>(m_fftCfg));
        m_fftCfg = nullptr;
    }
    delete m_decoder;
}

bool AudioAnalyzer::open(const QString &filePath, double startTime) {
    close();

    if (!m_decoder->open(filePath))
        return false;

    if (startTime > 0.0)
        m_decoder->seek(startTime);

    m_fftCfg = kiss_fftr_alloc(kFftSize, 0, nullptr, nullptr);
    if (!m_fftCfg) {
        m_decoder->close();
        return false;
    }

    std::fill(m_ringBuffer.begin(), m_ringBuffer.end(), 0.f);
    std::fill(m_spectrum.begin(), m_spectrum.end(), 0.f);
    std::fill(m_smoothedSpectrum.begin(), m_smoothedSpectrum.end(), 0.f);
    m_ringWrite = 0;
    m_ringFilled = 0;
    m_level = 0.f;
    m_lowBand = 0.f;
    m_midBand = 0.f;
    m_highBand = 0.f;
    m_beatPulse = 0.f;
    m_prevLowEnergy = 0.f;
    m_playheadSeconds = std::max(0.0, startTime);
    m_playbackSpeed = 1.0;
    m_hasData = false;
    return true;
}

void AudioAnalyzer::close() {
    if (m_fftCfg) {
        kiss_fftr_free(static_cast<kiss_fftr_cfg>(m_fftCfg));
        m_fftCfg = nullptr;
    }
    if (m_decoder)
        m_decoder->close();

    m_ringWrite = 0;
    m_ringFilled = 0;
    m_level = 0.f;
    m_lowBand = 0.f;
    m_midBand = 0.f;
    m_highBand = 0.f;
    m_beatPulse = 0.f;
    m_prevLowEnergy = 0.f;
    m_playheadSeconds = 0.0;
    m_playbackSpeed = 1.0;
    m_hasData = false;
}

bool AudioAnalyzer::seek(double seconds) {
    if (!m_decoder || !m_decoder->isOpen())
        return false;
    const double clamped = std::max(0.0, seconds);
    if (!m_decoder->seek(clamped))
        return false;

    std::fill(m_ringBuffer.begin(), m_ringBuffer.end(), 0.f);
    std::fill(m_spectrum.begin(), m_spectrum.end(), 0.f);
    std::fill(m_smoothedSpectrum.begin(), m_smoothedSpectrum.end(), 0.f);
    m_ringWrite = 0;
    m_ringFilled = 0;
    m_level = 0.f;
    m_lowBand = 0.f;
    m_midBand = 0.f;
    m_highBand = 0.f;
    m_beatPulse = 0.f;
    m_prevLowEnergy = 0.f;
    m_playheadSeconds = clamped;
    m_hasData = false;
    return true;
}

void AudioAnalyzer::setPlaybackSpeed(double speed) {
    if (!m_decoder)
        return;
    const double clamped = (speed > 0.01) ? speed : 1.0;
    m_playbackSpeed = clamped;
    m_decoder->setPlaybackSpeed(clamped);
}

void AudioAnalyzer::appendMonoSample(float sample) {
    m_ringBuffer[m_ringWrite] = sample;
    m_ringWrite = (m_ringWrite + 1) % kFftSize;
    m_ringFilled = std::min(m_ringFilled + 1, kFftSize);
}

void AudioAnalyzer::advance(double deltaSeconds) {
    if (!m_decoder->isOpen() || !m_fftCfg || deltaSeconds <= 0.0)
        return;

    deltaSeconds = std::min(deltaSeconds, 0.1);
    const int samplesNeeded = static_cast<int>(deltaSeconds * kSampleRate);

    int samplesDecoded = 0;
    double sumSq = 0.0;
    int levelCount = 0;

    while (samplesDecoded < samplesNeeded) {
        QByteArray chunk;
        if (!m_decoder->decodeNextChunk(chunk)) {
            if (m_decoder->atEnd()) {
                m_decoder->seek(0.0);
                continue;
            }
            break;
        }

        const auto *data = reinterpret_cast<const float *>(chunk.constData());
        const int numFrames = chunk.size() / (static_cast<int>(sizeof(float)) * AudioDecoder::kOutputChannels);
        for (int i = 0; i < numFrames && samplesDecoded < samplesNeeded; ++i) {
            const float mono = (data[i * 2] + data[i * 2 + 1]) * 0.5f;
            appendMonoSample(mono);
            sumSq += static_cast<double>(mono) * static_cast<double>(mono);
            ++levelCount;
            ++samplesDecoded;
        }
    }

    if (levelCount > 0) {
        const float rms = static_cast<float>(std::sqrt(sumSq / levelCount));
        m_level = std::clamp(rms * 4.f, 0.f, 1.f);
        const double decodedSeconds = static_cast<double>(samplesDecoded) / static_cast<double>(kSampleRate);
        m_playheadSeconds += decodedSeconds * m_playbackSpeed;
    }

    // Fast attack / slower release beat pulse envelope.
    const float releasePerSecond = 3.0f;
    m_beatPulse = std::max(0.0f, m_beatPulse - static_cast<float>(deltaSeconds) * releasePerSecond);

    if (m_ringFilled >= kFftSize)
        computeSpectrum();
}

void AudioAnalyzer::computeSpectrum() {
    for (int i = 0; i < kFftSize; ++i) {
        const int idx = (m_ringWrite + i) % kFftSize;
        m_fftInput[i] = m_ringBuffer[idx] * m_hannWindow[i];
    }

    std::vector<kiss_fft_cpx> freqData(kFftSize / 2 + 1);
    kiss_fftr(static_cast<kiss_fftr_cfg>(m_fftCfg),
              m_fftInput.data(),
              freqData.data());

    const int numFftBins = kFftSize / 2;
    std::vector<float> rawBins(kBins, 0.f);

    for (int b = 0; b < kBins; ++b) {
        const float t0 = static_cast<float>(b) / kBins;
        const float t1 = static_cast<float>(b + 1) / kBins;
        const int binLow  = std::max(1, static_cast<int>(std::pow(numFftBins, t0)));
        const int binHigh = std::min(numFftBins, static_cast<int>(std::pow(numFftBins, t1)));
        float sum = 0.f;
        int count = 0;
        for (int i = binLow; i <= binHigh; ++i) {
            const float re = freqData[i].r;
            const float im = freqData[i].i;
            sum += std::sqrt(re * re + im * im);
            ++count;
        }
        rawBins[b] = count > 0 ? sum / static_cast<float>(count) : 0.f;
    }

    float maxVal = 0.f;
    for (float v : rawBins)
        maxVal = std::max(maxVal, v);
    const float norm = maxVal > 1e-6f ? maxVal : 1.f;

    for (int b = 0; b < kBins; ++b) {
        const float normalized = std::clamp(rawBins[b] / norm, 0.f, 1.f);
        m_spectrum[b] = m_smoothedSpectrum[b] * 0.5f + normalized * 0.5f;
        m_smoothedSpectrum[b] = m_spectrum[b];
    }

    // Broad perceptual bands on normalized log bins:
    // low: kick/bass, mid: body/vocals, high: hats/detail.
    const int lowEnd = std::max(1, kBins / 8);
    const int midEnd = std::max(lowEnd + 1, (kBins * 5) / 8);

    float lowSum = 0.f;
    float midSum = 0.f;
    float highSum = 0.f;
    int lowCount = 0;
    int midCount = 0;
    int highCount = 0;
    for (int b = 0; b < kBins; ++b) {
        const float v = m_spectrum[b];
        if (b < lowEnd) {
            lowSum += v;
            ++lowCount;
        } else if (b < midEnd) {
            midSum += v;
            ++midCount;
        } else {
            highSum += v;
            ++highCount;
        }
    }

    const float lowRaw = (lowCount > 0) ? (lowSum / static_cast<float>(lowCount)) : 0.f;
    const float midRaw = (midCount > 0) ? (midSum / static_cast<float>(midCount)) : 0.f;
    const float highRaw = (highCount > 0) ? (highSum / static_cast<float>(highCount)) : 0.f;

    // Slightly different smoothing per band to preserve punch in lows.
    m_lowBand = m_lowBand * 0.68f + lowRaw * 0.32f;
    m_midBand = m_midBand * 0.78f + midRaw * 0.22f;
    m_highBand = m_highBand * 0.82f + highRaw * 0.18f;

    // Beat pulse from low-band transient.
    const float lowDelta = std::max(0.0f, m_lowBand - m_prevLowEnergy);
    const float dynamicThreshold = 0.06f + m_level * 0.08f;
    if (lowDelta > dynamicThreshold)
        m_beatPulse = std::max(m_beatPulse, std::clamp(lowDelta * 6.0f, 0.0f, 1.0f));
    m_prevLowEnergy = m_lowBand;

    m_hasData = true;
}
