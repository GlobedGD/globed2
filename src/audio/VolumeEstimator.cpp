#include <globed/audio/VolumeEstimator.hpp>
#include <globed/audio/AudioStream.hpp>

#include <asp/iter.hpp>
#include <qunet/util/algo.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

VolumeEstimator::VolumeEstimator(size_t sampleRate) : m_sampleRate(sampleRate) {}

void VolumeEstimator::feedData(const float* pcm, size_t samples) {
    m_sampleQueue.writeData(pcm, samples);
    m_writeHistory.emplace_back(Instant::now(), samples);

    if (m_sampleQueue.size() > static_cast<float>(m_sampleRate) * BUFFER_SIZE) {
        m_sampleQueue.clear();
        m_writeHistory.clear();
    }
}

static float calculatePcmVolume(const float* pcm, size_t samples) {
    if (samples == 0) return 0.f;

    double sum = asp::iter::from(pcm, samples).map([](float v) { return v * v; }).sum();

    return static_cast<float>(sqrt(sum / (double)samples));
}

void VolumeEstimator::update(float dt) {
    if (std::isnan(dt)) { // yes, this happens sometimes
        dt = 0.f;
    }

    dt = std::clamp(dt, 0.0f, 0.25f);

    const size_t needed = static_cast<size_t>(static_cast<float>(m_sampleRate) * BUFFER_SIZE * dt);
    float buf[needed];

    size_t copied = 0;

    // Now, determine which samples we should copy and which we shouldn't
    // We skip AUDIO_PLAYBACK_DELAY ms of samples, since this data hasn't been played yet
    auto now = Instant::now();
    while (copied < needed && !m_writeHistory.empty()) {
        auto& [time, count] = m_writeHistory.front();
        if (now.durationSince(time).seconds<float>() < AUDIO_PLAYBACK_DELAY) {
            break;
        }

        // these samples are ready to be used now
        size_t toCopy = std::min(count, needed - copied);
        size_t actuallyCopied = m_sampleQueue.readData(&buf[copied], toCopy);
        GLOBED_DEBUG_ASSERT(actuallyCopied == toCopy); // should always hold

        copied += actuallyCopied;
        count -= actuallyCopied;

        if (count == 0) {
            m_writeHistory.pop_front();
        }
    }

    if (copied < needed) {
        // fill the rest with void
        for (size_t i = copied; i < needed; i++) {
            buf[i] = 0.f;
        }
    }

    float newVolume = calculatePcmVolume(buf, needed);
    m_emaVolume = qn::exponentialMovingAverage(m_emaVolume, newVolume, 0.2);

    // this / 0.2f might need some tweaking
    m_normalizedVolume = std::sqrt(std::clamp(m_emaVolume / 0.25f, 0.f, 1.f));
}

float VolumeEstimator::getVolume() {
    return m_normalizedVolume;
}

}
