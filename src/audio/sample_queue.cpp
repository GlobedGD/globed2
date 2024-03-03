#include "sample_queue.hpp"

#ifdef GLOBED_VOICE_SUPPORT

void AudioSampleQueue::writeData(const DecodedOpusData& data) {
    this->writeData(data.ptr, data.length);
}

void AudioSampleQueue::writeData(const float* pcm, size_t length) {
    buf.insert(buf.end(), pcm, pcm + length);
}

size_t AudioSampleQueue::copyTo(float* dest, size_t samples) {
    if (buf.size() < samples) {
        size_t total = buf.size();

        std::copy(buf.data(), buf.data() + total, dest);
        buf.clear();
        return total;
    }

    std::copy(buf.data(), buf.data() + samples, dest);
    buf.erase(buf.begin(), buf.begin() + samples);

    return samples;
}

size_t AudioSampleQueue::size() const {
    return buf.size();
}

void AudioSampleQueue::clear() {
    buf.clear();
}

float* AudioSampleQueue::data() {
    return buf.data();
}

#endif // GLOBED_VOICE_SUPPORT