#include "opus_codec.hpp"

#if GLOBED_VOICE_SUPPORT

using namespace util::data;

OpusCodec::OpusCodec(int sampleRate, int frameSize) {
    setSampleRate(sampleRate);
    setFrameSize(frameSize);
}

OpusCodec::~OpusCodec() {
    cleanup();
}

void OpusCodec::setSampleRate(int sampleRate) {
    if (sampleRate == this->sampleRate) {
        return;
    }
    
    this->sampleRate = sampleRate;
    cleanup(); // free previous encoder and decoder

    if (sampleRate == 0) {
        return; // don't do anything
    }

    encoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_VOIP, &_res);
    
    errcheck("opus_encoder_create");

    decoder = opus_decoder_create(sampleRate, 1, &_res);
    errcheck("opus_decoder_create");
}

void OpusCodec::setFrameSize(int frameSize) {
    this->frameSize = frameSize;
}

EncodedOpusData OpusCodec::encode(const float* data) {
    EncodedOpusData out;
    size_t bytes = sizeof(short) * frameSize;
    out.length = bytes / 2;
    out.ptr = new byte[out.length];

    out.length = opus_encode_float(encoder, data, frameSize, out.ptr, out.length);
    if (out.length < 0) {
        delete[] out.ptr;
        _res = out.length;
        errcheck("opus_encode");
    }

    return out;
}

DecodedOpusData OpusCodec::decode(const byte* data, size_t length) {
    DecodedOpusData out;

    size_t outBytes = sizeof(float) * frameSize * 1; // channels = 1

    out.ptr = new float[outBytes];
    out.lengthBytes = outBytes;
    
    out.lengthSamples = opus_decode_float(decoder, data, length, out.ptr, frameSize, 0);
    if (out.lengthSamples < 0) {
        delete[] out.ptr;
        _res = out.lengthSamples;
        errcheck("opus_decode");
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
        GLOBED_ASSERT(false, std::string("opus error in ") + where + ": " + msg);
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