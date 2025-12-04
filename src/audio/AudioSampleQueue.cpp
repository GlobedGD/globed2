#include <globed/audio/AudioSampleQueue.hpp>

using namespace geode::prelude;

namespace globed {

void AudioSampleQueue::writeData(const float* data, size_t num) {
    m_queue.insert(m_queue.end(), data, data + num);

    if (m_limit != 0 && m_queue.size() > m_limit) {
        size_t excess = m_queue.size() - m_limit;
        m_queue.erase(m_queue.begin(), m_queue.begin() + excess);
    }
}

size_t AudioSampleQueue::readData(float* out, size_t max) {
    size_t toRead = std::min(max, m_queue.size());
    if (out) {
        this->peekData(out, toRead);
    }
    m_queue.erase(m_queue.begin(), m_queue.begin() + toRead);
    return toRead;
}

size_t AudioSampleQueue::peekData(float* out, size_t max) const {
    size_t toRead = std::min(max, m_queue.size());
    std::copy(m_queue.begin(), m_queue.begin() + toRead, out);
    return toRead;
}

size_t AudioSampleQueue::size() const {
    return m_queue.size();
}

void AudioSampleQueue::clear() {
    m_queue.clear();
}

std::optional<const float*> AudioSampleQueue::contiguousData() const {
    auto begin = &*m_queue.begin();
    auto end = &*m_queue.end();

    return end >= begin ? std::optional<const float*>{begin} : std::nullopt;
}

void AudioSampleQueue::setLimit(size_t limit) {
    m_limit = limit;
}

}