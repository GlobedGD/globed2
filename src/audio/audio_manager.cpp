#include "audio_manager.hpp"

#if GLOBED_VOICE_SUPPORT

#include <util/time.hpp>
#include <util/debugging.hpp>

#define FMOD_ERR_CHECK(res, msg) \
    auto GEODE_CONCAT(__evcond, __LINE__) = (res); \
    if (GEODE_CONCAT(__evcond, __LINE__) != FMOD_OK) GLOBED_REQUIRE(false, std::string(msg) + " failed: " + std::to_string((int)GEODE_CONCAT(__evcond, __LINE__)));

GLOBED_SINGLETON_DEF(GlobedAudioManager)

GlobedAudioManager::GlobedAudioManager() {
    audioThreadHandle = std::thread(&GlobedAudioManager::audioThreadFunc, this);

    opus.setSampleRate(VOICE_TARGET_SAMPLERATE);
    opus.setFrameSize(VOICE_TARGET_FRAMESIZE);

    recordDevice = {.id = -1};
    playbackDevice = {.id = -1};
}

GlobedAudioManager::~GlobedAudioManager() {
    _terminating = true;
}

std::vector<AudioRecordingDevice> GlobedAudioManager::getRecordingDevices() {
    std::vector<AudioRecordingDevice> out;

    int numDrivers, numConnected;
    FMOD_ERR_CHECK(
        this->getSystem()->getRecordNumDrivers(&numDrivers, &numConnected),
        "System::getRecordNumDrivers"
    );

    for (int i = 0; i < numDrivers; i++) {
        out.push_back(this->getRecordingDevice(i));
    }

    return out;
}

std::vector<AudioPlaybackDevice> GlobedAudioManager::getPlaybackDevices() {
    std::vector<AudioPlaybackDevice> out;

    int numDrivers;
    FMOD_ERR_CHECK(
        this->getSystem()->getNumDrivers(&numDrivers),
        "System::getNumDrivers"
    );

    for (int i = 0; i < numDrivers; i++) {
        out.push_back(this->getPlaybackDevice(i));
    }

    return out;
}

AudioRecordingDevice GlobedAudioManager::getRecordingDevice(int deviceId) {
    AudioRecordingDevice device;
    char name[256];
    FMOD_ERR_CHECK(this->getSystem()->getRecordDriverInfo(
        deviceId, name, 256,
        &device.guid,
        &device.sampleRate,
        &device.speakerMode, 
        &device.speakerModeChannels,
        &device.driverState
    ), "System::getRecordDriverInfo");
    
    device.id = deviceId;
    device.name = std::string(name);

    return device;
}

AudioPlaybackDevice GlobedAudioManager::getPlaybackDevice(int deviceId) {
    AudioPlaybackDevice device;
    char name[256];
    FMOD_ERR_CHECK(this->getSystem()->getDriverInfo(
        deviceId, name, 256,
        &device.guid,
        &device.sampleRate,
        &device.speakerMode, 
        &device.speakerModeChannels
    ), "System::getDriverInfo");

    device.id = deviceId;
    device.name = std::string(name);

    return device;
}

void GlobedAudioManager::startRecording(std::function<void(const EncodedAudioFrame&)> callback) {
    GLOBED_REQUIRE(this->recordDevice.id >= 0, "no recording device is set")
    GLOBED_REQUIRE(!isRecording(), "attempting to record when already recording");

    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.length = sizeof(float) * exinfo.numchannels * (int)((float)VOICE_TARGET_SAMPLERATE * VOICE_CHUNK_RECORD_TIME);

    recordChunkSize = exinfo.length;

    FMOD_ERR_CHECK(
        this->getSystem()->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &recordSound),
        "System::createSound"
    );

    std::lock_guard lock(recordMutex);

    recordContinueStream();

    recordActive = true;
    recordCallback = callback;
}

void GlobedAudioManager::stopRecording() {
    recordMutex.lock();

    recordActive = false;
    recordCallback = [](const auto& _){};

    recordMutex.unlock();

    // wait for audio thread to finish tasks
    while (!audioThreadSleeping) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::lock_guard lock(recordMutex);

    if (recordSound) {
        recordSound->release();
        recordSound = nullptr;
    }
}

void GlobedAudioManager::queueStopRecording() {
    recordQueuedStop = true;
}

void GlobedAudioManager::recordContinueStream() {
    FMOD_ERR_CHECK(
        this->getSystem()->recordStart(recordDevice.id, recordSound, false),
        "System::recordStart"
    );
}

bool GlobedAudioManager::isRecording() {
    GLOBED_REQUIRE(this->recordDevice.id >= 0, "no recording device is set")
    bool recording;
    FMOD_ERR_CHECK(
        this->getSystem()->isRecording(this->recordDevice.id, &recording),
        "System::isRecording"
    );
    return recording;
}

void GlobedAudioManager::playSound(FMOD::Sound* sound) {
    FMOD::Channel* ch = nullptr;
    FMOD_ERR_CHECK(
        this->getSystem()->playSound(sound, nullptr, false, &ch),
        "System::playSound"  
    );
}

FMOD::Sound* GlobedAudioManager::createSound(const float* pcm, size_t samples, int sampleRate) {
    FMOD_CREATESOUNDEXINFO exinfo = {};
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = sampleRate;
    exinfo.length = sizeof(float) * samples;

    FMOD::Sound* sound;

    FMOD_ERR_CHECK(this->getSystem()->createSound(
        nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &sound),
        "System::createSound"
    );

    float* data;
    FMOD_ERR_CHECK(
        sound->lock(0, exinfo.length, (void**)&data, nullptr, nullptr, nullptr),
        "Sound::lock"
    );

    std::memcpy(data, pcm, exinfo.length);

    FMOD_ERR_CHECK(
        sound->unlock(data, nullptr, exinfo.length, 0),
        "Sound::unlock"
    );

    return sound;
}

DecodedOpusData GlobedAudioManager::decodeSound(const EncodedOpusData& data) {
    return opus.decode(data.ptr, data.length);
}

DecodedOpusData GlobedAudioManager::decodeSound(util::data::byte* data, size_t length) {
    return opus.decode(data, length);
}

AudioRecordingDevice GlobedAudioManager::getRecordingDevice() {
    return recordDevice;
}

AudioPlaybackDevice GlobedAudioManager::getPlaybackDevice() {
    return playbackDevice;
}

void GlobedAudioManager::setActiveRecordingDevice(int deviceId) {
    recordDevice = this->getRecordingDevice(deviceId);
}

void GlobedAudioManager::setActivePlaybackDevice(int deviceId) {
    playbackDevice = this->getPlaybackDevice(deviceId);
}


void GlobedAudioManager::audioThreadFunc() {
    while (!_terminating) {
        if (!recordActive) {
            audioThreadSleeping = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (recordQueuedStop) {
            recordQueuedStop = false;
            stopRecording();
            continue;
        }

        audioThreadSleeping = false;

        if (!this->isRecording()) {
            // chunk is available, process it
            std::lock_guard lock(recordMutex);

            EncodedAudioFrame frame;

            float* pcmData;
            unsigned int pcmLen;

            FMOD_ERR_CHECK(
                recordSound->lock(0, recordChunkSize, (void**)&pcmData, nullptr, &pcmLen, nullptr),
                "Sound::lock"
            );

            float* tmpBuf = new float[pcmLen / sizeof(float)];
            std::memcpy(tmpBuf, pcmData, pcmLen);

            FMOD_ERR_CHECK(
                recordSound->unlock(pcmData, nullptr, pcmLen, 0),
                "Sound::unlock"
            );

            recordContinueStream();

            try {
                size_t totalOpusFrames = static_cast<float>(VOICE_TARGET_SAMPLERATE) / VOICE_TARGET_FRAMESIZE * VOICE_CHUNK_RECORD_TIME;

                for (size_t i = 0; i < totalOpusFrames; i++) {
                    const float* dataStart = tmpBuf + i * VOICE_TARGET_FRAMESIZE;
                    auto encodedFrame = opus.encode(dataStart);
                    frame.pushOpusFrame(std::move(encodedFrame));
                }
            } catch (const std::exception& e) {
                delete[] tmpBuf;
                geode::log::error("Ignoring exception in audio thread: {}", e.what());
                continue;
            }

            delete[] tmpBuf;

            try {
                recordCallback(frame);
            } catch (const std::exception& e) {
                geode::log::error("ignoring exception in audio callback: {}", e.what());
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }
}

FMOD::System* GlobedAudioManager::getSystem() {
    if (!cachedSystem) {
        cachedSystem = FMODAudioEngine::sharedEngine()->m_system;
    }

    return cachedSystem;
}

#endif // GLOBED_VOICE_SUPPORT