#include "encoder.hpp"

#if GLOBED_VOICE_SUPPORT

#include <opus.h>

using namespace util::data;

AudioEncoder::AudioEncoder(int sampleRate, int frameSize, int channels) {
    this->frameSize = frameSize;
    this->sampleRate = sampleRate;
    this->channels = channels;
    (void) this->remakeEncoder().unwrap();
}

AudioEncoder::~AudioEncoder() {
    if (encoder) {
        opus_encoder_destroy(encoder);
    }
}

AudioEncoder::AudioEncoder(AudioEncoder&& other) noexcept {
    encoder = other.encoder;
    other.encoder = nullptr;

    channels = other.channels;
    sampleRate = other.sampleRate;
    frameSize = other.frameSize;
}

AudioEncoder& AudioEncoder::operator=(AudioEncoder&& other) noexcept {
    if (this != &other) {
        if (this->encoder) {
            opus_encoder_destroy(this->encoder);
        }

        this->encoder = other.encoder;
        other.encoder = nullptr;

        channels = other.channels;
        sampleRate = other.sampleRate;
        frameSize = other.frameSize;
    }

    return *this;
}

Result<EncodedOpusData> AudioEncoder::encode(const float* data) {
    EncodedOpusData out;
    size_t bytes = sizeof(float) * frameSize / 4; // the /4 is arbitrary, could experiment with it
    out.ptr = new byte[bytes];

    out.length = opus_encode_float(encoder, data, frameSize, out.ptr, bytes);
    if (out.length < 0) {
        delete[] out.ptr;
        _res = out.length;
        GLOBED_UNWRAP(this->errcheck("opus_encode_float"));
    }

    return Ok(out);
}

Result<> AudioEncoder::setSampleRate(int sampleRate) {
    this->sampleRate = sampleRate;
    return this->remakeEncoder();
}

void AudioEncoder::setFrameSize(int frameSize) {
    this->frameSize = frameSize;
}

Result<> AudioEncoder::setChannels(int channels) {
    this->channels = channels;
    return this->remakeEncoder();
}

Result<> AudioEncoder::resetState() {
    _res = opus_encoder_ctl(encoder, OPUS_RESET_STATE);
    return this->errcheck("AudioEncoder::resetState");
}

Result<> AudioEncoder::setBitrate(int bitrate) {
    _res = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
    return this->errcheck("AudioEncoder::setBitrate");
}

Result<> AudioEncoder::setComplexity(int complexity) {
    _res = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(complexity));
    return this->errcheck("AudioEncoder::setComplexity");
}

Result<> AudioEncoder::setVariableBitrate(bool variablebr) {
    _res = opus_encoder_ctl(encoder, OPUS_SET_VBR(variablebr ? 1 : 0));
    return this->errcheck("AudioEncoder::setVariableBitrate");
}

Result<> AudioEncoder::remakeEncoder() {
    // if we are reinitializing, free the previous encoder
    if (encoder) {
        opus_encoder_destroy(encoder);
        encoder = nullptr;
    }

    // sampleRate 0 is valid and considered as a cleanup of the encoder
    if (sampleRate == 0) {
        return Ok();
    }

    encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &_res);
    return this->errcheck("opus_encoder_create");
}

Result<> AudioEncoder::errcheck(const char* where) {
    if (_res != OPUS_OK) {
        const char* msg = opus_strerror(_res);
        GLOBED_REQUIRE_SAFE(false, std::string("opus error in ") + where + ": " + msg)
    }

    return Ok();
}

#endif // GLOBED_VOICE_SUPPORT