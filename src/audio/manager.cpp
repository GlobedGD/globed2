#include "manager.hpp"

#if GLOBED_VOICE_SUPPORT

#include <managers/error_queues.hpp>

#define FMOD_ERR_CHECK(res, msg) \
    do { \
        auto _res = (res); \
        GLOBED_REQUIRE(_res == FMOD_OK, GlobedAudioManager::formatFmodError(_res, msg)); \
    } while (0); \

GlobedAudioManager::GlobedAudioManager()
    : encoder(VOICE_TARGET_SAMPLERATE, VOICE_TARGET_FRAMESIZE, VOICE_CHANNELS) {

    audioThreadHandle = std::thread(&GlobedAudioManager::audioThreadFunc, this);

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
    )

    for (int i = 0; i < numDrivers; i++) {
        auto dev = this->getRecordingDevice(i);
        if (dev.has_value()) {
            out.push_back(dev.value());
        }
    }

    return out;
}

std::vector<AudioPlaybackDevice> GlobedAudioManager::getPlaybackDevices() {
    std::vector<AudioPlaybackDevice> out;

    int numDrivers;
    FMOD_ERR_CHECK(
        this->getSystem()->getNumDrivers(&numDrivers),
        "System::getNumDrivers"
    )

    for (int i = 0; i < numDrivers; i++) {
        auto dev = this->getPlaybackDevice(i);
        if (dev.has_value()) {
            out.push_back(dev.value());
        }
    }

    return out;
}

std::optional<AudioRecordingDevice> GlobedAudioManager::getRecordingDevice(int deviceId) {
    AudioRecordingDevice device;
    char name[256];
    if (this->getSystem()->getRecordDriverInfo(
        deviceId, name, 256,
        &device.guid,
        &device.sampleRate,
        &device.speakerMode,
        &device.speakerModeChannels,
        &device.driverState
    ) != FMOD_OK) {
        return std::nullopt;
    }

    device.id = deviceId;
    device.name = std::string(name);

    return device;
}

std::optional<AudioPlaybackDevice> GlobedAudioManager::getPlaybackDevice(int deviceId) {
    AudioPlaybackDevice device;
    char name[256];
    if (this->getSystem()->getDriverInfo(
        deviceId, name, 256,
        &device.guid,
        &device.sampleRate,
        &device.speakerMode,
        &device.speakerModeChannels
    ) != FMOD_OK) {
        return std::nullopt;
    }

    device.id = deviceId;
    device.name = std::string(name);

    return device;
}

bool GlobedAudioManager::isRecordingDeviceSet() {
    return recordDevice.id != -1;
}

void GlobedAudioManager::validateDevices() {
    if (recordDevice.id != -1) {
        this->setActiveRecordingDevice(recordDevice.id);
        if (recordDevice.id == -1) {
            geode::log::info("Invalidating recording device {}", recordDevice.id);
        }
    }

    if (playbackDevice.id != -1) {
        this->setActivePlaybackDevice(playbackDevice.id);
        if (playbackDevice.id == -1) {
            geode::log::info("Invalidating playback device {}", playbackDevice.id);
        }
    }
}

void GlobedAudioManager::startRecording(std::function<void(const EncodedAudioFrame&)> callback) {
    GLOBED_REQUIRE(this->recordDevice.id >= 0, "no recording device is set")
    GLOBED_REQUIRE(!this->isRecording() && !recordActive, "attempting to record when already recording")

    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.length = sizeof(float) * exinfo.defaultfrequency * exinfo.numchannels;

    recordChunkSize = exinfo.length;

    FMOD_ERR_CHECK(
        this->getSystem()->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &exinfo, &recordSound),
        "System::createSound"
    )

    FMOD_ERR_CHECK(
        this->getSystem()->recordStart(recordDevice.id, recordSound, true),
        "System::recordStart"
    )

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
    )

    // if halting instead of stopping, don't call the callback
    if (recordQueuedHalt) {
        recordFrame.clear();
        recordQueuedHalt = false;
    } else {
        // call the callback if there's any audio leftover
        this->recordInvokeCallback();
    }

    // cleanup
    recordCallback = [](const auto&){};
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
    )

    return recording;
}

FMOD::Channel* GlobedAudioManager::playSound(FMOD::Sound* sound) {
    FMOD::Channel* ch = nullptr;
    FMOD_ERR_CHECK(
        this->getSystem()->playSound(sound, nullptr, false, &ch),
        "System::playSound"
    )

    return ch;
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
    )

    float* data;
    FMOD_ERR_CHECK(
        sound->lock(0, exinfo.length, (void**)&data, nullptr, nullptr, nullptr),
        "Sound::lock"
    )

    std::memcpy(data, pcm, exinfo.length);

    FMOD_ERR_CHECK(
        sound->unlock(data, nullptr, exinfo.length, 0),
        "Sound::unlock"
    )

    return sound;
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

    auto dev = this->getRecordingDevice(deviceId);
    if (dev.has_value()) {
        recordDevice = dev.value();
    } else {
        recordDevice.id = -1;
    }
}

void GlobedAudioManager::setActivePlaybackDevice(int deviceId) {
    auto dev = this->getPlaybackDevice(deviceId);
    if (dev.has_value()) {
        playbackDevice = dev.value();
    } else {
        playbackDevice.id = -1;
    }
}

void GlobedAudioManager::recordInvokeCallback() {
    if (recordFrame.size() == 0) return;

    try {
        recordCallback(recordFrame);
    } catch(const std::exception& e) {
        ErrorQueues::get().error(std::string("Exception in audio callback: ") + e.what());
    }

    recordFrame.clear();
}

void GlobedAudioManager::audioThreadFunc() {
    while (!_terminating) {
        // if we are not recording right now, sleep
        if (!recordActive) {
            audioThreadSleeping = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
        )

        // if we are at the same position, do nothing
        if (pos == recordLastPosition) {
            this->getSystem()->update();
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            continue;
        }

        FMOD_ERR_CHECK(
            recordSound->lock(0, recordChunkSize, (void**)&pcmData, nullptr, &pcmLen, nullptr),
            "Sound::lock"
        )

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
        )

        if (recordQueue.size() >= VOICE_TARGET_FRAMESIZE) {
            float pcmbuf[VOICE_TARGET_FRAMESIZE];
            recordQueue.copyTo(pcmbuf, VOICE_TARGET_FRAMESIZE);

            try {
                recordFrame.pushOpusFrame(encoder.encode(pcmbuf));
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
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        // std::this_thread::yield();
    }
}

FMOD::System* GlobedAudioManager::getSystem() {
    if (!cachedSystem) {
        cachedSystem = FMODAudioEngine::sharedEngine()->m_system;
    }

    return cachedSystem;
}

const char* GlobedAudioManager::fmodErrorString(FMOD_RESULT result) {
    static constexpr const char* values[] = {
        "No error",
        "Tried to call a function on a data type that does not allow this type of functionality",
        "Error trying to allocate a Channel",
        "The specified Channel has been reused to play another Sound",
        "DMA Failure",
        "DSP connection error (possibly caused by a cyclic dependency or a connected DSPs with incompatible buffer counts)",
        "DSP don't process (return value for a DSP callback)",
        "DSP Format error",
        "DSP is already in the mixer's DSP network",
        "Ccould not find the DSP unit specified",
        "Cannot perform operation on this DSP as it is reserved by the system",
        "DSP silence (return value for a DSP callback)",
        "DSP operation cannot be performed on a DSP of this type",
        "Error loading file",
        "Couldn't perform a seek operation",
        "Media was ejected while reading",
        "End of file unexpectedly reached while trying to read essential data",
        "End of current chunk reached while trying to read data",
        "File not found",
        "Unsupported file or audio format",
        "Version mismatch between the FMOD header and either the FMOD Studio library or the FMOD Core library",
        "An HTTP error occurred",
        "The specified resource requires authentication or is forbidden",
        "Proxy authentication is required to access the specified resource",
        "An HTTP server error occurred",
        "The HTTP request timed out",
        "FMOD was not initialized correctly to support this function",
        "Cannot call this command after the system has already been initialized",
        "An internal FMOD error occurred",
        "Value passed in was a NaN, Inf or denormalized float",
        "An invalid object handle was used",
        "An invalid parameter was passed to this function",
        "An invalid seek position was passed to this function",
        "An invalid speaker was passed to this function based on the current speaker mode",
        "The syncpoint did not come from this Sound handle",
        "Tried to call a function on a thread that is not supported",
        "The vectors passed in are not unit length, or perpendicular",
        "Reached maximum audible playback count for this Sound's SoundGroup",
        "Not enough memory or resources",
        "Can't use FMOD_OPENMEMORY_POINT on non PCM source data, or non mp3/xma/adpcm data if FMOD_CREATECOMPRESSEDSAMPLE was used",
        "Tried to call a command on a 2D Sound when the command was meant for 3D Sound",
        "Tried to use a feature that requires hardware support",
        "Couldn't connect to the specified host",
        "A socket error occurred",
        "The specified URL couldn't be resolved",
        "Operation on a non-blocking socket could not complete immediately",
        "Operation could not be performed because specified Sound/DSP connection is not ready",
        "Error initializing output device, because it is already in use and cannot be reused",
        "Error creating hardware sound buffer",
        "A call to a standard soundcard driver failed, which could possibly mean a bug in the driver or resources were missing or exhausted",
        "Soundcard does not support the specified format",
        "Error initializing output device",
        "The output device has no drivers installed",
        "An unspecified error has been returned from a plugin",
        "A requested output, DSP unit type or codec was not available",
        "A resource that the plugin requires cannot be allocated or found",
        "A plugin was built with an unsupported SDK version",
        "An error occurred trying to initialize the recording device",
        "Reverb properties cannot be set on this Channel because a parent ChannelGroup owns the reverb connection",
        "Specified instance in FMOD_REVERB_PROPERTIES couldn't be set. Most likely because it is an invalid instance number or the reverb doesn't exist",
        "Sound contains subsounds when it shouldn't have, or it doesn't contain subsounds when it should have. The operation may also not be able to be performed on a parent Sound",
        "This subsound is already being used by another Sound, you cannot have more than one parent to a Sound",
        "Shared subsounds cannot be replaced or moved from their parent stream, such as when the parent stream is an FSB file",
        "The specified tag could not be found or there are no tags",
        "The Sound created exceeds the allowable input channel count. This can be increased using the 'maxinputchannels' parameter in System::setSoftwareFormat",
        "The retrieved string is too long to fit in the supplied buffer and has been truncated",
        "Something in FMOD hasn't been implemented when it should be!",
        "This command cannot be called before initializing the System",
        "A command issued was not supported by this object",
        "The version number of this file format is not supported",
        "The specified bank has already been loaded",
        "The live update connection failed due to the game already being connected",
        "The live update connection failed due to the game data being out of sync with the tool",
        "The live update connection timed out",
        "The requested event, parameter, bus or vca could not be found",
        "The Studio::System object is not yet initialized",
        "The specified resource is not loaded, so it can't be unloaded",
        "An invalid string was passed to this function",
        "The specified resource is already locked",
        "The specified resource is not locked, so it can't be unlocked",
        "The specified recording driver has been disconnected",
        "The length provided exceeds the allowable limit"
    };

    int idx = static_cast<int>(result);

    if (idx < 0 || idx > 81) {
        return "Invalid FMOD error code was passed; there is no corresponding error message to return";
    }

    return values[idx];
}

std::string GlobedAudioManager::formatFmodError(FMOD_RESULT result, const char* whatFailed) {
    return fmt::format("{} failed: [FMOD error {}] {}", whatFailed, (int)result, fmodErrorString(result));
}

#endif // GLOBED_VOICE_SUPPORT
