#include "encoder.hpp"

#if GLOBED_VOICE_SUPPORT

using namespace util::data;

AudioEncoder::AudioEncoder(int sampleRate, int frameSize, int channels) {
    this->frameSize = frameSize;
    this->sampleRate = sampleRate;
    this->channels = channels;
    this->remakeEncoder();
}

AudioEncoder::~AudioEncoder() {
    if (encoder) {
        opus_encoder_destroy(encoder);
    }
}

EncodedOpusData AudioEncoder::encode(const float* data) {
    EncodedOpusData out;
    size_t bytes = sizeof(float) * frameSize / 4; // the /4 is arbitrary, could experiment with it
    out.ptr = new byte[bytes];

    out.length = opus_encode_float(encoder, data, frameSize, out.ptr, bytes);
    if (out.length < 0) {
        delete[] out.ptr;
        _res = out.length;
        this->errcheck("opus_encode_float");
    }

    return out;
}

void AudioEncoder::setSampleRate(int sampleRate) {
    this->sampleRate = sampleRate;
    this->remakeEncoder();
}

void AudioEncoder::setFrameSize(int frameSize) {
    this->frameSize = frameSize;
}

void AudioEncoder::setChannels(int channels) {
    this->channels = channels;
    this->remakeEncoder();
}

void AudioEncoder::resetState() {
    _res = opus_encoder_ctl(encoder, OPUS_RESET_STATE);
    this->errcheck("AudioEncoder::resetState");
}

void AudioEncoder::setBitrate(int bitrate) {
    _res = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
    this->errcheck("AudioEncoder::setBitrate");
}

void AudioEncoder::setComplexity(int complexity) {
    _res = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(complexity));
    this->errcheck("AudioEncoder::setComplexity");
}

void AudioEncoder::setVariableBitrate(bool variablebr) {
    _res = opus_encoder_ctl(encoder, OPUS_SET_VBR(variablebr ? 1 : 0));
    this->errcheck("AudioEncoder::setVariableBitrate");
}

void AudioEncoder::remakeEncoder() {
    // if we are reinitializing, free the previous encoder
    if (encoder) {
        opus_encoder_destroy(encoder);
        encoder = nullptr;
    }

    // sampleRate 0 is valid and considered as a cleanup of the encoder
    if (sampleRate == 0) {
        return;
    }

    encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &_res);
    this->errcheck("opus_encoder_create");
}

void AudioEncoder::errcheck(const char* where) {
    if (_res != OPUS_OK) {
        const char* msg = opus_strerror(_res);
        GLOBED_REQUIRE(false, std::string("opus error in ") + where + ": " + msg)
    }
}

#endif // GLOBED_VOICE_SUPPORT