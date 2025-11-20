#include <globed/audio/AudioDecoder.hpp>
#include <globed/audio/AudioEncoder.hpp>

#include <opus.h>

using namespace geode::prelude;

namespace globed {

AudioDecoder::AudioDecoder(int sampleRate, int frameSize, int channels)
    : m_decoder(nullptr), m_sampleRate(sampleRate), m_frameSize(frameSize), m_channels(channels) {
    (void) this->remakeDecoder().unwrap();
}

AudioDecoder::~AudioDecoder() {
    if (m_decoder) {
        opus_decoder_destroy(m_decoder);
    }
}

AudioDecoder::AudioDecoder(AudioDecoder&& other) noexcept {
    m_decoder = other.m_decoder;
    other.m_decoder = nullptr;

    m_channels = other.m_channels;
    m_sampleRate = other.m_sampleRate;
    m_frameSize = other.m_frameSize;
}

AudioDecoder& AudioDecoder::operator=(AudioDecoder&& other) noexcept {
    if (this != &other) {
        if (m_decoder) {
            opus_decoder_destroy(m_decoder);
        }

        m_decoder = other.m_decoder;
        other.m_decoder = nullptr;

        m_channels = other.m_channels;
        m_sampleRate = other.m_sampleRate;
        m_frameSize = other.m_frameSize;
    }

    return *this;
}

Result<DecodedOpusData> AudioDecoder::decode(const uint8_t* data, size_t length) {
    DecodedOpusData out;

    out.size = m_frameSize * m_channels;
    out.data = std::make_shared<float[]>(out.size);

    int result = opus_decode_float(m_decoder, data, length, out.data.get(), m_frameSize, 0);

    if (result < 0) {
        return Err("opus_decode_float failed: {}", AudioDecoder::errorToString(result));
    }

    return Ok(out);
}

Result<DecodedOpusData> AudioDecoder::decode(const EncodedOpusData& frame) {
    return this->decode(frame.data.get(), frame.size);
}

Result<> AudioDecoder::setSampleRate(int sampleRate) {
    m_sampleRate = sampleRate;
    return this->remakeDecoder();
}

void AudioDecoder::setFrameSize(int frameSize) {
    m_frameSize = frameSize;
}

Result<> AudioDecoder::setChannels(int channels) {
    m_channels = channels;
    return this->remakeDecoder();
}

Result<> AudioDecoder::resetState() {
    int res = opus_decoder_ctl(m_decoder, OPUS_RESET_STATE);
    if (res < 0) {
        return Err("opus_decoder_ctl(resetState) failed: {}", AudioDecoder::errorToString(res));
    }

    return Ok();
}

Result<> AudioDecoder::remakeDecoder() {
    // if we are reinitializing, free the previous decoder
    if (m_decoder) {
        opus_decoder_destroy(m_decoder);
        m_decoder = nullptr;
    }

    // sampleRate 0 is valid and considered as a cleanup of the decoder
    if (m_sampleRate == 0) {
        return Ok();
    }

    int res;
    m_decoder = opus_decoder_create(m_sampleRate, m_channels, &res);

    if (res != OPUS_OK) {
        return Err("opus_decoder_create failed: {}", AudioDecoder::errorToString(res));
    }

    return Ok();
}

std::string_view AudioDecoder::errorToString(int code) {
    return opus_strerror(code);
}

}
