#pragma once

#include <cstddef>
#include <deque>
#include <globed/config.hpp>
#include <optional>

namespace globed {

class GLOBED_DLL AudioSampleQueue {
public:
    AudioSampleQueue() = default;

    AudioSampleQueue(const AudioSampleQueue &) = default;
    AudioSampleQueue(AudioSampleQueue &&) = default;
    AudioSampleQueue &operator=(const AudioSampleQueue &) = default;
    AudioSampleQueue &operator=(AudioSampleQueue &&) = default;

    void writeData(const float *data, size_t count);
    size_t readData(float *out, size_t max);
    size_t peekData(float *out, size_t max) const;
    size_t size() const;
    void clear();

    std::optional<const float *> contiguousData() const;

    void setLimit(size_t limit);

private:
    size_t m_limit = 0;
    std::deque<float> m_queue;
};

} // namespace globed