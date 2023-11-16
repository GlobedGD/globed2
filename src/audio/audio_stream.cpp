#include "audio_stream.hpp"

#if GLOBED_VOICE_SUPPORT

#include "audio_frame.hpp"
#include "audio_manager.hpp"

#include <util/time.hpp>
#include <util/debugging.hpp>

/* AudioSampleQueue */

void AudioSampleQueue::writeData(DecodedOpusData data) {
    buf.insert(buf.end(), data.ptr, data.ptr + data.length);
}

size_t AudioSampleQueue::copyTo(float* dest, size_t samples) {
    size_t total;

    if (buf.size() < samples) {
        total = buf.size();

        std::copy(buf.data(), buf.data() + buf.size(), dest);
        buf.clear();
        return total;
    }

    std::copy(buf.data(), buf.data() + samples, dest);
    buf.erase(buf.begin(), buf.begin() + samples);

    return samples;
}

size_t AudioSampleQueue::size() {
    return buf.size();
}

/* AudioStream */

AudioStream::AudioStream() {
    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.userdata = this;
    exinfo.length = sizeof(float) * exinfo.numchannels * exinfo.defaultfrequency * (VOICE_CHUNK_RECORD_TIME / 2);

    // geode::log::debug("{}: creating stream, length: {}", util::time::nowPretty(), exinfo.length);
    exinfo.pcmreadcallback = [](FMOD_SOUND* sound_, void* data, unsigned int len) -> FMOD_RESULT {
        FMOD::Sound* sound = reinterpret_cast<FMOD::Sound*>(sound_);
        AudioStream* stream = nullptr;
        sound->getUserData((void**)&stream);

        if (!stream) {
            geode::log::debug("audio stream is nullptr in cb, ignoring");
            return FMOD_OK;
        }

        // geode::log::debug("cb called, size: {}", len / sizeof(float));

        // write data..

        size_t neededSamples = len / sizeof(float);
        size_t copied = stream->queue.copyTo((float*)data, neededSamples);

        if (copied != neededSamples) {
            geode::log::debug("{}: could not match samples, needed: {}, got: {}", util::time::nowPretty(), neededSamples, copied);
            // fill the rest with the void to not repeat stuff
            for (size_t i = copied; i < neededSamples; i++) {
                ((float*)data)[i] = 0.0f;
            }
        } else {
            // geode::log::debug("{}: pulled {} samples, queue size: {}", util::time::nowPretty(), copied, stream->queue.size());
        }

        return FMOD_OK;
    };
    
    auto& vm = GlobedAudioManager::get();
    
    FMOD_RESULT res;
    auto system = vm.getSystem();
    res = system->createStream(nullptr, FMOD_OPENUSER | FMOD_2D | FMOD_LOOP_NORMAL, &exinfo, &sound);

    GLOBED_REQUIRE(res == FMOD_OK, std::string("FMOD System::createStream failed: ") + std::to_string((int)res));
}

AudioStream::~AudioStream() {
    geode::log::debug("releasing sound");
    // TODO fix this wtf
    // if (sound) sound->release();
}

AudioStream::AudioStream(AudioStream&& other) {
    this->sound = other.sound;
    this->queue = other.queue;
}

void AudioStream::start() {
    GlobedAudioManager::get().playSound(sound);
}

void AudioStream::writeData(const EncodedAudioFrame& frame) {
    auto& vm = GlobedAudioManager::get();
    FMOD_RESULT res;

    const auto& frames = frame.extractFrames();
    for (const auto& opusFrame : frames) {
        auto decodedFrame = vm.decodeSound(opusFrame);

        queue.writeData(decodedFrame);
        OpusCodec::freeData(decodedFrame);
    }
}

#endif // GLOBED_VOICE_SUPPORT