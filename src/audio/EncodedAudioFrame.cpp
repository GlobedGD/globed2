#include <globed/audio/EncodedAudioFrame.hpp>

using namespace geode::prelude;

namespace globed {

EncodedAudioFrame::EncodedAudioFrame() : m_capacity(VOICE_MAX_FRAMES_IN_AUDIO_FRAME) {}

EncodedAudioFrame::EncodedAudioFrame(size_t capacity) : m_capacity(capacity) {}

EncodedAudioFrame::~EncodedAudioFrame()
{
    this->clear();
}

Result<> EncodedAudioFrame::pushOpusFrame(const EncodedOpusData &frame)
{
    if (m_frames.size() >= m_capacity && m_capacity) {
        return Err("EncodedAudioFrame is full (capacity {})", m_capacity);
    }

    m_frames.push_back(frame);
    return Ok();
}

void EncodedAudioFrame::setCapacity(size_t frames)
{
    m_capacity = frames;
}

void EncodedAudioFrame::clear()
{
    m_frames.clear();
}

size_t EncodedAudioFrame::size() const
{
    return m_frames.size();
}

size_t EncodedAudioFrame::capacity() const
{
    return m_capacity;
}

const std::vector<EncodedOpusData> &EncodedAudioFrame::getFrames() const
{
    return m_frames;
}

} // namespace globed