#include "manager.hpp"

#include <defs/geode.hpp>

#ifdef GLOBED_VOICE_SUPPORT

#ifdef GEODE_IS_WINDOWS
# include <objbase.h>
#endif

#include <opus.h>
#include <fmod_errors.h>
#include <Geode/utils/permission.hpp>

#include <globed/tracing.hpp>
#include <managers/error_queues.hpp>
#include <managers/settings.hpp>
#include <util/debug.hpp>
#include <util/format.hpp>

using namespace geode::prelude;
namespace permission = geode::utils::permission;
using permission::Permission;

#define FMOD_ERR_CHECK(res, msg) \
    do { \
        auto _res = (res); \
        GLOBED_REQUIRE(_res == FMOD_OK, GlobedAudioManager::formatFmodError(_res, msg)); \
    } while (0); \

#define FMOD_ERR_CHECK_SAFE(res, msg) \
    do { \
        auto _res = (res); \
        GLOBED_REQUIRE_SAFE(_res == FMOD_OK, GlobedAudioManager::formatFmodError(_res, msg)); \
    } while (0); \


GlobedAudioManager::GlobedAudioManager()
    : encoder(VOICE_TARGET_SAMPLERATE, VOICE_TARGET_FRAMESIZE, VOICE_CHANNELS) {

    audioThreadHandle.setLoopFunction(&GlobedAudioManager::audioThreadFunc);

    // initializing COM is not necessary as FMOD will do it on its own, but FMOD docs recommend doing it anyway.
    audioThreadHandle.setStartFunction([] {
        geode::utils::thread::setName("Audio Thread");
#ifdef GEODE_IS_WINDOWS
        auto result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (result != S_OK) {
            log::error("failed to initialize COM: {:X}", result);
        }
#endif
    });

#ifdef GEODE_IS_WINDOWS
    audioThreadHandle.setTerminationFunction([] {
        CoUninitialize();
    });
#endif

    audioThreadHandle.start(this);

    recordDevice = AudioRecordingDevice{.id = -1};
    playbackDevice = AudioPlaybackDevice{.id = -1};
}

GlobedAudioManager::~GlobedAudioManager() {
    TRACE("[AudioManager] waiting for the thread to halt");

    audioThreadHandle.stopAndWait();

    TRACE("[AudioManager] audio thread halted");
}

void GlobedAudioManager::preInitialize() {
#ifdef GEODE_IS_ANDROID
    // the first call to FMOD::System::getRecordDriverInfo for some reason can take half a second on android,
    // causing a freeze when the user first opens the playlayer.
    // to avoid that, we call this once upon loading the mod, so the freeze happens on the loading screen instead.
    try {
        this->getRecordingDevice(0);
    } catch (const std::exception _e) {}
#endif

    auto& settings = GlobedSettings::get();

    // also check perms on android
    if (!permission::getPermissionStatus(Permission::RecordAudio)) {
        settings.communication.audioDevice = -1;
    }

    // load the previously selected device
    if (settings.communication.audioDevice == -1) {
        recordDevice = std::nullopt;
        return;
    }

    auto device = this->getRecordingDevice(settings.communication.audioDevice);

    if (device.has_value()) {
        recordDevice = std::move(device.value());
        return;
    }

    // if invalid, try to get the first device in the list
    device = this->getRecordingDevice(0);
    if (device.has_value()) {
        recordDevice = std::move(device.value());
    } else {
        // give up.
        recordDevice = std::nullopt;
    }
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

    if (!loopbacksAllowed && (
        device.name.find("[loopback]") != std::string::npos
        || device.name.find("(loopback)") != std::string::npos
        || device.name.find("Monitor of") != std::string::npos
    )) {
        return std::nullopt;
    }

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
    return recordDevice.has_value();
}

void GlobedAudioManager::validateDevices() {
    if (this->isRecording()) {
        log::warn("Attempting to invoke validateDevices while recording audio");
        return;
    }

    if (recordDevice.has_value()) {
        // check if this device still exists
        if (auto dev = this->getRecordingDevice(recordDevice->id)) {
            recordDevice = dev;
        } else {
            log::warn("Invalidating audio device {} ({})", recordDevice->id, recordDevice->name);
            recordDevice = std::nullopt;
        }
    }

    if (playbackDevice.has_value()) {
        // check if this device still exists
        if (auto dev = this->getPlaybackDevice(playbackDevice->id)) {
            playbackDevice = dev;
        } else {
            log::warn("Invalidating audio device {} ({})", playbackDevice->id, playbackDevice->name);
            playbackDevice = std::nullopt;
        }
    }
}

void GlobedAudioManager::setRecordBufferCapacity(size_t frames) {
    recordFrame.setCapacity(frames);
}

Result<> GlobedAudioManager::startRecordingInternal(bool passive) {
    if (!permission::getPermissionStatus(Permission::RecordAudio)) {
        return Err("Recording failed, please grant microphone permission in Globed settings");
    }

    GLOBED_REQUIRE_SAFE(this->recordDevice.has_value(), "no recording device is set")
    GLOBED_REQUIRE_SAFE(!this->isRecording() && !recordActive, "attempting to record when already recording")

    if (recordSound != nullptr) {
        recordSound->release();
        recordSound = nullptr;
    }

    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.length = sizeof(float) * exinfo.defaultfrequency * exinfo.numchannels;

    recordChunkSize = exinfo.length;

    FMOD_ERR_CHECK_SAFE(
        this->getSystem()->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &exinfo, &recordSound),
        "System::createSound"
    )

    FMOD_RESULT res = this->getSystem()->recordStart(recordDevice->id, recordSound, true);

    // invalid device most likely
    if (res == FMOD_ERR_RECORD) {
        return Err("Failed to start recording audio");
    }

    FMOD_ERR_CHECK_SAFE(res, "System::recordStart")

    recordQueuedStop = false;
    recordQueuedHalt = false;
    recordLastPosition = 0;
    recordActive = true;
    recordingPassive = passive;

    return Ok();
}

Result<> GlobedAudioManager::startRecording(std::function<void(const EncodedAudioFrame&)> callback) {
    auto result = this->startRecordingInternal();
    if (result.isErr()) return result;

    recordCallback = callback;
    recordingRaw = false;

    return Ok();
}

Result<> GlobedAudioManager::startRecordingRaw(std::function<void(const float*, size_t)> callback) {
    auto result = this->startRecordingInternal();
    if (result.isErr()) return result;

    recordRawCallback = callback;
    recordingRaw = true;

    return Ok();
}

void GlobedAudioManager::internalStopRecording(bool ignoreErrors) {
    if (ignoreErrors) {
        (void) this->getSystem()->recordStop(recordDevice->id);
    } else {
        FMOD_ERR_CHECK(
            this->getSystem()->recordStop(recordDevice->id),
            "System::recordStop"
        )
    }

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
    recordRawCallback = [](const auto*, auto) {};
    recordLastPosition = 0;
    recordChunkSize = 0;
    recordingRaw = false;
    recordingPassive = false;
    recordingPassiveActive = false;
    recordQueue.clear();

    if (recordSound) {
        recordSound->release();
        recordSound = nullptr;
    }

    recordActive = false;
    recordQueuedStop = false;
}

void GlobedAudioManager::stopRecording() {
    recordQueuedStop = true;
}

void GlobedAudioManager::haltRecording() {
    recordQueuedStop = true;
    recordQueuedHalt = true;
}

bool GlobedAudioManager::isRecording() {
    if (!this->recordDevice) {
        return false;
    }

    if (recordActive && recordingPassive) {
        return recordingPassiveActive;
    }

    bool recording;

    if (FMOD_OK != this->getSystem()->isRecording(this->recordDevice->id, &recording)) {
        this->recordDevice = std::nullopt;
        this->internalStopRecording(true);
        return false;
    }

    return recording;
}

Result<> GlobedAudioManager::startPassiveRecording(std::function<void(const EncodedAudioFrame&)> callback) {
    auto result = this->startRecordingInternal(true);
    if (result.isErr()) return result;

    recordCallback = callback;
    recordingRaw = false;

    return Ok();
}

void GlobedAudioManager::resumePassiveRecording() {
    recordingPassiveActive = true;
}

void GlobedAudioManager::pausePassiveRecording() {
    recordingPassiveActive = false;
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

const std::optional<AudioRecordingDevice>& GlobedAudioManager::getRecordingDevice() {
    return recordDevice;
}

const std::optional<AudioPlaybackDevice>& GlobedAudioManager::getPlaybackDevice() {
    return playbackDevice;
}

void GlobedAudioManager::setActiveRecordingDevice(int deviceId) {
    auto dev = this->getRecordingDevice(deviceId);

    if (dev.has_value()) {
        this->setActiveRecordingDevice(dev.value());
    }
}

void GlobedAudioManager::setActiveRecordingDevice(const AudioRecordingDevice& device) {
    if (this->isRecording()) {
        ErrorQueues::get().warn("[Globed] attempting to change the recording device while recording");
        return;
    }

    recordDevice = device;
}

void GlobedAudioManager::setActivePlaybackDevice(int deviceId) {
    auto dev = this->getPlaybackDevice(deviceId);

    if (dev.has_value()) {
        this->setActivePlaybackDevice(dev.value());
    }
}

void GlobedAudioManager::setActivePlaybackDevice(const AudioPlaybackDevice& device) {
    playbackDevice = device;
}

void GlobedAudioManager::toggleLoopbacksAllowed(bool allowed) {
    loopbacksAllowed = allowed;
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

void GlobedAudioManager::recordInvokeRawCallback(float* pcm, size_t samples) {
    if (samples == 0) return;

    try {
        recordRawCallback(pcm, samples);
    } catch (const std::exception& e) {
        ErrorQueues::get().error(std::string("Exception in raw audio callback: ") + e.what());
    }
}

void GlobedAudioManager::audioThreadFunc(decltype(audioThreadHandle)::StopToken&) {
    // if we are not recording right now, sleep
    if (!recordActive) {
        audioThreadSleeping = true;
        asp::time::sleep(asp::time::Duration::fromMillis(5));
        return;
    }

    // if someone queued us to stop recording, back to sleeping
    if (recordQueuedStop) {
        recordQueuedStop = false;
        audioThreadSleeping = true;
        this->internalStopRecording();
        return;
    }

    audioThreadSleeping = false;

    auto result = this->audioThreadWork();

    if (result.isErr()) {
        ErrorQueues::get().warn(result.unwrapErr());
        audioThreadSleeping = true;
        this->internalStopRecording();
    }
}

Result<> GlobedAudioManager::audioThreadWork() {
    float* pcmData;
    unsigned int pcmLen;

    unsigned int pos;
    FMOD_ERR_CHECK_SAFE(
        this->getSystem()->getRecordPosition(recordDevice->id, &pos),
        "System::getRecordPosition"
    )

    // if we are at the same position, do nothing
    if (pos == recordLastPosition) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return Ok();
    }

    FMOD_ERR_CHECK_SAFE(
        recordSound->lock(0, recordChunkSize, (void**)&pcmData, nullptr, &pcmLen, nullptr),
        "Sound::lock"
    )

    // don't write any data if we are in passive recording and not currently recording
    if (!recordingPassive || recordingPassiveActive) {
        if (pos > recordLastPosition) {
            recordQueue.writeData(pcmData + recordLastPosition, pos - recordLastPosition);
        } else if (pos < recordLastPosition) { // we have reached the end of the buffer
            // write the data left at the end
            recordQueue.writeData(pcmData + recordLastPosition, pcmLen / sizeof(float) - recordLastPosition);
            // write the data from beginning to current pos
            recordQueue.writeData(pcmData, pos);
        }
    }

    recordLastPosition = pos;

    FMOD_ERR_CHECK_SAFE(
        recordSound->unlock(pcmData, nullptr, pcmLen, 0),
        "Sound::unlock"
    )

    if (recordingRaw) {
        // raw recording, call the raw callback with the pcm data directly.
        float* pcm = recordQueue.data();
        size_t samples = recordQueue.size();
        this->recordInvokeRawCallback(pcm, samples);
        recordQueue.clear();
    } else {
        // encoded recording, encode the data and push to the frame.
        if (recordQueue.size() >= VOICE_TARGET_FRAMESIZE) {
            float pcmbuf[VOICE_TARGET_FRAMESIZE];
            recordQueue.copyTo(pcmbuf, VOICE_TARGET_FRAMESIZE);

            GLOBED_UNWRAP_INTO(encoder.encode(pcmbuf), auto opusFrame);
            GLOBED_UNWRAP(recordFrame.pushOpusFrame(opusFrame));
        }

        // if we are at capacity, or we just stopped passive recording, call the callback
        if (recordFrame.size() >= recordFrame.capacity() || (recordFrame.size() > 0 && recordingPassive && !recordingPassiveActive)) {
            this->recordInvokeCallback();
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    // std::this_thread::yield();

    return Ok();
}

FMOD::System* GlobedAudioManager::getSystem() {
    if (!cachedSystem) {
        cachedSystem = FMODAudioEngine::sharedEngine()->m_system;
    }

    return cachedSystem;
}

const char* GlobedAudioManager::fmodErrorString(FMOD_RESULT result) {
    return FMOD_ErrorString(result);
}

std::string GlobedAudioManager::formatFmodError(FMOD_RESULT result, const char* whatFailed) {
    return fmt::format("{} failed: [FMOD error {}] {}", whatFailed, (int)result, fmodErrorString(result));
}

const char* GlobedAudioManager::getOpusVersion() {
    return opus_get_version_string();
}

#else // GLOBED_VOICE_SUPPORT

bool GlobedAudioManager::isRecording() {
    return false;
}

#endif // GLOBED_VOICE_SUPPORT
