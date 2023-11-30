#include "opus_codec.hpp"

#if GLOBED_VOICE_SUPPORT

using namespace util::data;

OpusCodec::OpusCodec(int sampleRate, int frameSize) {
    this->setSampleRate(sampleRate);
    this->setFrameSize(frameSize);
}

OpusCodec::~OpusCodec() {
    this->cleanup();
}

void OpusCodec::setSampleRate(int sampleRate) {
    if (sampleRate == this->sampleRate) {
        return;
    }

    this->sampleRate = sampleRate;
    this->cleanup(); // free previous encoder and decoder

    if (sampleRate == 0) {
        return; // don't do anything
    }

    encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &_res);
    this->errcheck("opus_encoder_create");

    decoder = opus_decoder_create(sampleRate, channels, &_res);
    this->errcheck("opus_decoder_create");
}

void OpusCodec::setFrameSize(int frameSize) {
    this->frameSize = frameSize;
}

EncodedOpusData OpusCodec::encode(const float* data) {
    EncodedOpusData out;
    size_t bytes = sizeof(float) * frameSize / 4;
    out.length = bytes;
    out.ptr = new byte[bytes];

    out.length = opus_encode_float(encoder, data, frameSize, out.ptr, bytes);
    if (out.length < 0) {
        delete[] out.ptr;
        _res = out.length;
        this->errcheck("opus_encode_float");
    }

    return out;
}

DecodedOpusData OpusCodec::decode(const byte* data, size_t length) {
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


void OpusCodec::freeData(DecodedOpusData data) {
    delete[] data.ptr;
}

void OpusCodec::freeData(EncodedOpusData data) {
    delete[] data.ptr;
}

void OpusCodec::errcheck(const char* where) {
    if (_res != OPUS_OK) {
        const char* msg = opus_strerror(_res);
        GLOBED_REQUIRE(false, std::string("opus error in ") + where + ": " + msg);
    }
}

void OpusCodec::cleanup() {
    if (encoder) {
        opus_encoder_destroy(encoder);
        encoder = nullptr;
    }

    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }
}

#endif // GLOBED_VOICE_SUPPORT