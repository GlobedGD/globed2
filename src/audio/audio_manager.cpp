#include "audio_manager.hpp"

#if GLOBED_VOICE_SUPPORT

#define FMOD_ERR_CHECK(res, msg) \
    auto GEODE_CONCAT(__evcond, __LINE__) = (res); \
    if (GEODE_CONCAT(__evcond, __LINE__) != FMOD_OK) GLOBED_ASSERT(false, std::string(msg) + " failed: " + std::to_string((int)GEODE_CONCAT(__evcond, __LINE__)));

GLOBED_SINGLETON_DEF(GlobedAudioManager)

GlobedAudioManager::GlobedAudioManager() {
    audioThreadHandle = std::thread(&GlobedAudioManager::audioThreadFunc, this);

    opus.setSampleRate(VOICE_TARGET_SAMPLERATE);
    opus.setFrameSize(VOICE_TARGET_FRAMESIZE);
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
    GLOBED_ASSERT(!isRecording(), "attempting to record when already recording");

    FMOD_CREATESOUNDEXINFO exinfo;
    std::memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = recordDevice.sampleRate;
    exinfo.length = sizeof(float) * exinfo.numchannels * (int)((float)recordDevice.sampleRate * VOICE_CHUNK_RECORD_TIME);

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
    recordCallback = [](auto&& _){};

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

void GlobedAudioManager::recordContinueStream() {
    FMOD_ERR_CHECK(
        this->getSystem()->recordStart(recordDevice.id, recordSound, false),
        "System::recordStart"
    );
}

bool GlobedAudioManager::isRecording() {
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
    recordResampler.setSampleRate(recordDevice.sampleRate, VOICE_TARGET_SAMPLERATE);
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

            try {
                float* resampled = recordResampler.resample(pcmData, (int)((float)recordDevice.sampleRate * VOICE_CHUNK_RECORD_TIME));

                size_t totalOpusFrames = static_cast<float>(VOICE_TARGET_SAMPLERATE) / VOICE_TARGET_FRAMESIZE * VOICE_CHUNK_RECORD_TIME;

                for (size_t i = 0; i < totalOpusFrames; i++) {
                    const float* dataStart = resampled + i * VOICE_TARGET_FRAMESIZE;
                    auto encodedFrame = opus.encode(dataStart);
                    frame.pushOpusFrame(std::move(encodedFrame));
                }
            } catch (const std::exception& e) {
                recordSound->unlock(pcmData, nullptr, pcmLen, 0);
                geode::log::error("Ignoring exception in audio thread: {}", e.what());
                continue;
            }

            FMOD_ERR_CHECK(
                recordSound->unlock(pcmData, nullptr, pcmLen, 0),
                "Sound::unlock"
            );

            recordCallback(frame);
            recordContinueStream();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

FMOD::System* GlobedAudioManager::getSystem() {
    return FMODAudioEngine::sharedEngine()->m_system;
}

#endif // GLOBED_VOICE_SUPPORT