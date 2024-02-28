#include "decoder.hpp"

#if GLOBED_VOICE_SUPPORT

#include <opus.h>

using namespace util::data;

void DecodedOpusData::freeData() {
    GLOBED_REQUIRE(ptr != nullptr, "attempting to double free an instance of DecodedOpusData")

    delete[] ptr;
#ifdef GLOBED_DEBUG
    // to try and prevent misuse
    ptr = nullptr;
    length = -1; // wraps around to int max
#endif // GLOBED_DEBUG
}

AudioDecoder::AudioDecoder(int sampleRate, int frameSize, int channels) {
    this->frameSize = frameSize;
    this->sampleRate = sampleRate;
    this->channels = channels;
    (void) this->remakeDecoder().unwrap();
}

AudioDecoder::~AudioDecoder() {
    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }
}

AudioDecoder::AudioDecoder(AudioDecoder&& other) noexcept {
    decoder = other.decoder;
    other.decoder = nullptr;

    channels = other.channels;
    sampleRate = other.sampleRate;
    frameSize = other.frameSize;
}

AudioDecoder& AudioDecoder::operator=(AudioDecoder&& other) noexcept {
    if (this != &other) {
        if (this->decoder) {
            opus_decoder_destroy(this->decoder);
        }

        this->decoder = other.decoder;
        other.decoder = nullptr;

        channels = other.channels;
        sampleRate = other.sampleRate;
        frameSize = other.frameSize;
    }

    return *this;
}

Result<DecodedOpusData> AudioDecoder::decode(const byte* data, size_t length) {
    DecodedOpusData out;

    out.length = frameSize * channels;
    out.ptr = new float[out.length];

    _res = opus_decode_float(decoder, data, length, out.ptr, frameSize, 0);

    if (_res < 0) {
        delete[] out.ptr;
        GLOBED_UNWRAP(this->errcheck("opus_decode_float"));
    }

    return Ok(out);
}

Result<DecodedOpusData> AudioDecoder::decode(const EncodedOpusData& data) {
    return this->decode(data.ptr, data.length);
}

Result<> AudioDecoder::setSampleRate(int sampleRate) {
    this->sampleRate = sampleRate;
    return this->remakeDecoder();
}

void AudioDecoder::setFrameSize(int frameSize) {
    this->frameSize = frameSize;
}

Result<> AudioDecoder::setChannels(int channels) {
    this->channels = channels;
    return this->remakeDecoder();
}

Result<> AudioDecoder::resetState() {
    _res = opus_decoder_ctl(decoder, OPUS_RESET_STATE);
    return this->errcheck("AudioDecoder::resetState");
}

Result<> AudioDecoder::remakeDecoder() {
    // if we are reinitializing, free the previous decoder
    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }

    // sampleRate 0 is valid and considered as a cleanup of the decoder
    if (sampleRate == 0) {
        return Ok();
    }

    decoder = opus_decoder_create(sampleRate, channels, &_res);
    return this->errcheck("opus_decoder_create");
}

Result<> AudioDecoder::errcheck(const char* where) {
    if (_res != OPUS_OK) {
        const char* msg = opus_strerror(_res);
        GLOBED_REQUIRE_SAFE(false, std::string("opus error in ") + where + ": " + msg)
    }

    return Ok();
}


#endif // GLOBED_VOICE_SUPPORT