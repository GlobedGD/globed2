#include <globed/audio/AudioEncoder.hpp>

#include <opus.h>

using namespace geode::prelude;

namespace globed {

AudioEncoder::AudioEncoder(int sampleRate, int frameSize, int channels)
    : m_encoder(nullptr), m_sampleRate(sampleRate), m_frameSize(frameSize), m_channels(channels) {
    (void) this->remakeEncoder().unwrap();
}

AudioEncoder::~AudioEncoder() {
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
    }
}

AudioEncoder::AudioEncoder(AudioEncoder&& other) noexcept {
    m_encoder = other.m_encoder;
    other.m_encoder = nullptr;

    m_channels = other.m_channels;
    m_sampleRate = other.m_sampleRate;
    m_frameSize = other.m_frameSize;
}

AudioEncoder& AudioEncoder::operator=(AudioEncoder&& other) noexcept {
    if (this != &other) {
        if (m_encoder) {
            opus_encoder_destroy(m_encoder);
        }

        m_encoder = other.m_encoder;
        other.m_encoder = nullptr;

        m_channels = other.m_channels;
        m_sampleRate = other.m_sampleRate;
        m_frameSize = other.m_frameSize;
    }

    return *this;
}

Result<EncodedOpusData> AudioEncoder::encode(const float* data) {
    EncodedOpusData out;
    size_t bytes = sizeof(float) * m_frameSize / 2; // arbitrary size

    auto buf = std::make_unique<uint8_t[]>(bytes);

    int result = opus_encode_float(m_encoder, data, m_frameSize, buf.get(), bytes);
    if (result < 0) {
        return Err("opus_encode_float failed: {}", AudioEncoder::errorToString(result));
    }

    out.data = std::move(buf);
    out.size = static_cast<size_t>(result);

    return Ok(std::move(out));
}

Result<> AudioEncoder::setSampleRate(int sampleRate) {
    m_sampleRate = sampleRate;
    return this->remakeEncoder();
}

void AudioEncoder::setFrameSize(int frameSize) {
    m_frameSize = frameSize;
}

Result<> AudioEncoder::setChannels(int channels) {
    m_channels = channels;
    return this->remakeEncoder();
}

Result<> AudioEncoder::resetState() {
    return this->encoderCtl("resetState", OPUS_RESET_STATE);
}

Result<> AudioEncoder::setBitrate(int bitrate) {
    return this->encoderCtl("setBitrate", OPUS_SET_BITRATE(bitrate));
}

Result<> AudioEncoder::setComplexity(int complexity) {
    return this->encoderCtl("setComplexity", OPUS_SET_COMPLEXITY(complexity));
}

Result<> AudioEncoder::setVariableBitrate(bool variablebr) {
    return this->encoderCtl("setVariableBitrate", OPUS_SET_VBR(variablebr ? 1 : 0));
}

Result<> AudioEncoder::remakeEncoder() {
    // if we are reinitializing, free the previous encoder
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
    }

    // sampleRate 0 is valid and considered as a cleanup of the encoder
    if (m_sampleRate == 0) {
        return Ok();
    }

    int err;
    m_encoder = opus_encoder_create(m_sampleRate, m_channels, OPUS_APPLICATION_VOIP, &err);

    if (err != OPUS_OK) {
        return Err("opus_encoder_create failed: {}", AudioEncoder::errorToString(err));
    }

    return Ok();
}

std::string_view AudioEncoder::errorToString(int code) {
    return opus_strerror(code);
}

}
