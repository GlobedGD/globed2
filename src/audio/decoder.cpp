#include "decoder.hpp"

#if GLOBED_VOICE_SUPPORT

using namespace util::data;

AudioDecoder::AudioDecoder(int sampleRate, int frameSize, int channels) {
    this->frameSize = frameSize;
    this->sampleRate = sampleRate;
    this->channels = channels;
    this->remakeDecoder();
}

AudioDecoder::~AudioDecoder() {
    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }
}

DecodedOpusData AudioDecoder::decode(const byte* data, size_t length) {
    DecodedOpusData out;

    out.length = frameSize * channels;
    out.ptr = new float[out.length];

    _res = opus_decode_float(decoder, data, length, out.ptr, frameSize, 0);

    if (_res < 0) {
        delete[] out.ptr;
        this->errcheck("opus_decode_float");
    }

    return out;
}

DecodedOpusData AudioDecoder::decode(const EncodedOpusData& data) {
    return this->decode(data.ptr, data.length);
}

void AudioDecoder::setSampleRate(int sampleRate) {
    this->sampleRate = sampleRate;
    this->remakeDecoder();
}

void AudioDecoder::setFrameSize(int frameSize) {
    this->frameSize = frameSize;
}

void AudioDecoder::setChannels(int channels) {
    this->channels = channels;
    this->remakeDecoder();
}

void AudioDecoder::resetState() {
    _res = opus_decoder_ctl(decoder, OPUS_RESET_STATE);
    this->errcheck("AudioDecoder::resetState");
}

void AudioDecoder::remakeDecoder() {
    // if we are reinitializing, free the previous decoder
    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }

    // sampleRate 0 is valid and considered as a cleanup of the decoder
    if (sampleRate == 0) {
        return;
    }

    decoder = opus_decoder_create(sampleRate, channels, &_res);
    this->errcheck("opus_decoder_create");
}

void AudioDecoder::errcheck(const char* where) {
    if (_res != OPUS_OK) {
        const char* msg = opus_strerror(_res);
        GLOBED_REQUIRE(false, std::string("opus error in ") + where + ": " + msg)
    }
}


#endif // GLOBED_VOICE_SUPPORT