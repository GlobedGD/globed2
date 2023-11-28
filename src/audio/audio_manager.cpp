#include "audio_manager.hpp"

#if GLOBED_VOICE_SUPPORT

#include <util/time.hpp>
#include <util/debugging.hpp>
#include <managers/error_queues.hpp>

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
    if (audioThreadHandle.joinable()) audioThreadHandle.join();

    geode::log::debug("audio thread halted.");
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

bool GlobedAudioManager::isRecordingDeviceSet() {
    return recordDevice.id != -1;
}

void GlobedAudioManager::validateDevices() {
    if (recordDevice.id != -1) {
        try {
            recordDevice = this->getRecordingDevice(recordDevice.id);
        } catch (const std::exception& e) {
            geode::log::info("Invalidating recording device {}: {}", recordDevice.id, e.what());
        }
    }

    if (playbackDevice.id != -1) {
        try {
            playbackDevice = this->getPlaybackDevice(playbackDevice.id);
        } catch (const std::exception& e) {
            geode::log::info("Invalidating playback device {}: {}", playbackDevice.id, e.what());
        }
    }
}

void GlobedAudioManager::startRecording(std::function<void(const EncodedAudioFrame&)> callback) {
    GLOBED_REQUIRE(this->recordDevice.id >= 0, "no recording device is set")
    GLOBED_REQUIRE(!this->isRecording() && !recordActive, "attempting to record when already recording");

    FMOD_CREATESOUNDEXINFO exinfo = {};

    // TODO figure it out in 2.2. the size is erroneously calculated as 144 on android.
#ifdef GLOBED_ANDROID
    exinfo.cbsize = 140;
#else
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
#endif

    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.length = sizeof(float) * exinfo.defaultfrequency * exinfo.numchannels;

    recordChunkSize = exinfo.length;

    FMOD_ERR_CHECK(
        this->getSystem()->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &exinfo, &recordSound),
        "System::createSound"
    );

    FMOD_ERR_CHECK(
        this->getSystem()->recordStart(recordDevice.id, recordSound, true),
        "System::recordStart"
    );

    recordQueuedStop = false;
    recordQueuedHalt = false;
    recordLastPosition = 0;
    recordCallback = callback;
    recordActive = true;
}

void GlobedAudioManager::internalStopRecording() {
    FMOD_ERR_CHECK(
        this->getSystem()->recordStop(recordDevice.id),
        "System::recordStop"
    );

    // if halting instead of stopping, don't call the callback
    if (recordQueuedHalt) {
        recordFrame.clear();
        recordQueuedHalt = false;
    } else {
        // call the callback if there's any audio leftover
        this->recordInvokeCallback();
    }

    // cleanup
    recordCallback = [](const auto& _){};
    recordLastPosition = 0;
    recordChunkSize = 0;
    recordQueue.clear();

    if (recordSound) {
        recordSound->release();
        recordSound = nullptr;
    }

    recordActive = false;
}

void GlobedAudioManager::stopRecording() {
    recordQueuedStop = true;
}

void GlobedAudioManager::haltRecording() {
    recordQueuedStop = true;
    recordQueuedHalt = true;
}

bool GlobedAudioManager::isRecording() {
    if (this->recordDevice.id == -1) {
        return false;
    }

    bool recording;

    FMOD_ERR_CHECK(
        this->getSystem()->isRecording(this->recordDevice.id, &recording),
        "System::isRecording"
    );

    return recording;
}

FMOD::Channel* GlobedAudioManager::playSound(FMOD::Sound* sound) {
    FMOD::Channel* ch = nullptr;
    FMOD_ERR_CHECK(
        this->getSystem()->playSound(sound, nullptr, false, &ch),
        "System::playSound"
    );

    return ch;
}

FMOD::Sound* GlobedAudioManager::createSound(const float* pcm, size_t samples, int sampleRate) {
    FMOD_CREATESOUNDEXINFO exinfo = {};
    // TODO figure it out in 2.2. the size is erroneously calculated as 144 on android.
#ifdef GLOBED_ANDROID
    exinfo.cbsize = 140;
#else
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
#endif

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
    if (recordDevice.id != -1) {
        GLOBED_REQUIRE(!this->isRecording(), "attempting to change the recording device while recording")
    }

    recordDevice = this->getRecordingDevice(deviceId);
}

void GlobedAudioManager::setActivePlaybackDevice(int deviceId) {
    playbackDevice = this->getPlaybackDevice(deviceId);
}

void GlobedAudioManager::recordInvokeCallback() {
    if (recordFrame.size() == 0) return;

    try {
        recordCallback(recordFrame);
    } catch (const std::exception& e) {
        ErrorQueues::get().error(std::string("Exception in audio callback: ") + e.what());
    }

    recordFrame.clear();
}

void GlobedAudioManager::audioThreadFunc() {
    while (!_terminating) {
        // if we are not recording right now, sleep
        if (!recordActive) {
            audioThreadSleeping = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        // if someone queued us to stop recording, back to sleeping
        if (recordQueuedStop) {
            recordQueuedStop = false;
            audioThreadSleeping = true;
            this->internalStopRecording();
            continue;
        }

        audioThreadSleeping = false;

        float* pcmData;
        unsigned int pcmLen;

        unsigned int pos;
        FMOD_ERR_CHECK(
            this->getSystem()->getRecordPosition(recordDevice.id, &pos),
            "System::getRecordPosition"
        );

        // if we are at the same position, do nothing
        if (pos == recordLastPosition) {
            this->getSystem()->update();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        FMOD_ERR_CHECK(
            recordSound->lock(0, recordChunkSize, (void**)&pcmData, nullptr, &pcmLen, nullptr),
            "Sound::lock"
        );

        if (pos > recordLastPosition) {
            recordQueue.writeData(pcmData + recordLastPosition, pos - recordLastPosition);
        } else if (pos < recordLastPosition) { // we have reached the end of the buffer
            // write the data left at the end
            recordQueue.writeData(pcmData + recordLastPosition, pcmLen / sizeof(float) - recordLastPosition);
            // write the data from beginning to current pos
            recordQueue.writeData(pcmData, pos);
        }

        recordLastPosition = pos;

        FMOD_ERR_CHECK(
            recordSound->unlock(pcmData, nullptr, pcmLen, 0),
            "Sound::unlock"
        );

        if (recordQueue.size() >= VOICE_TARGET_FRAMESIZE) {
            float pcmbuf[VOICE_TARGET_FRAMESIZE];
            recordQueue.copyTo(pcmbuf, VOICE_TARGET_FRAMESIZE);

            try {
                recordFrame.pushOpusFrame(opus.encode(pcmbuf));
            } catch (const std::exception& e) {
                ErrorQueues::get().error(std::string("Exception in audio thread: ") + e.what());
                continue;
            }
        }

        if (recordFrame.size() >= recordFrame.capacity()) {
            this->recordInvokeCallback();
        }

        this->getSystem()->update();

        // TODO maybe do something with this i dunno
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        // std::this_thread::yield();
    }
}

FMOD::System* GlobedAudioManager::getSystem() {
    if (!cachedSystem) {
        cachedSystem = FMODAudioEngine::sharedEngine()->m_system;
    }

    return cachedSystem;
}

#endif // GLOBED_VOICE_SUPPORT