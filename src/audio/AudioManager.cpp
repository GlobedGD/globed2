#include <globed/audio/AudioManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/util/format.hpp>

#ifdef GEODE_IS_WINDOWS
# include <objbase.h>
#endif

#include <fmod_errors.h>
#include <Geode/utils/permission.hpp>
#include <asp/time/sleep.hpp>

using namespace geode::prelude;
namespace permission = geode::utils::permission;
using permission::Permission;

static std::string formatFmodError(FMOD_RESULT result, const char* whatFailed) {
    return fmt::format("{} failed: [FMOD error {}] {}", whatFailed, (int)result, FMOD_ErrorString(result));
}

#define FMOD_ERRC(res, msg) \
    do { \
        auto _res = (res); \
        if (_res != FMOD_OK) return Err(formatFmodError(_res, msg)); \
    } while (0) \

namespace globed {

AudioManager::AudioManager()
    : m_encoder(VOICE_TARGET_SAMPLERATE, VOICE_TARGET_FRAMESIZE, VOICE_CHANNELS) {

    m_thread.setLoopFunction(&AudioManager::audioThreadFunc);

    // initializing COM is not necessary as FMOD will do it on its own, but FMOD docs recommend doing it anyway.
    m_thread.setStartFunction([] {
#ifdef GEODE_IS_WINDOWS
        auto result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (result != S_OK) {
            log::error("failed to initialize COM: {:X}", result);
        }
#endif
    });

#ifdef GEODE_IS_WINDOWS
    m_thread.setTerminationFunction([] {
        CoUninitialize();
    });
#endif

    m_thread.setName("Audio Thread");
    m_thread.start(this);

    m_recordDevice = AudioRecordingDevice{.id = -1};
}

AudioManager::~AudioManager() {
    m_thread.stopAndWait();

    for (auto& [_, stream] : m_playbackStreams) {
        // crash fix :p
        std::ignore = stream.release();
    }
}

void AudioManager::preInitialize() {
#ifdef GEODE_IS_ANDROID
    // the first call to FMOD::System::getRecordDriverInfo for some reason can take half a second on android,
    // causing a freeze when the user first opens the playlayer.
    // to avoid that, we call this once upon loading the mod, so the freeze happens on the loading screen instead.
    try {
        this->getRecordingDevice(0);
    } catch (const std::exception& _e) {}
#endif

    int deviceId = globed::setting<int>("core.audio.input-device");

    // also check perms on android
    if (!permission::getPermissionStatus(Permission::RecordAudio)) {
        deviceId = -1;
    }

    if (deviceId == -1) {
        m_recordDevice = std::nullopt;
        return;
    }

    auto device = this->getRecordingDevice(deviceId);
    if (device) {
        m_recordDevice = *device;
        return;
    }

    // if invalid, try to get the first device in the list
    device = this->getRecordingDevice(0);
    if (device.has_value()) {
        m_recordDevice = std::move(device.value());
    } else {
        // give up.
        m_recordDevice = std::nullopt;
    }
}

std::vector<AudioRecordingDevice> AudioManager::getRecordingDevices() {
    std::vector<AudioRecordingDevice> out;

    int numDrivers, numConnected;
    FMOD_RESULT err = this->getSystem()->getRecordNumDrivers(&numDrivers, &numConnected);
    if (FMOD_OK != err) {
        log::error("FMOD::System::getRecordNumDrivers failed: {}", FMOD_ErrorString(err));
        return out;
    }

    for (int i = 0; i < numDrivers; i++) {
        auto dev = this->getRecordingDevice(i);
        if (dev.has_value()) {
            out.push_back(dev.value());
        }
    }

    return out;
}

std::optional<AudioRecordingDevice> AudioManager::getRecordingDevice(int deviceId) {
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

    if (!m_loopbacksAllowed && (
        device.name.find("[loopback]") != std::string::npos
        || device.name.find("(loopback)") != std::string::npos
        || device.name.find("Monitor of") != std::string::npos
    )) {
        return std::nullopt;
    }

    return device;
}

const std::optional<AudioRecordingDevice>& AudioManager::getRecordingDevice() {
    return m_recordDevice;
}

bool AudioManager::isRecordingDeviceSet() {
    return m_recordDevice.has_value();
}

void AudioManager::validateDevices() {
    if (this->isRecording()) {
        log::warn("Attempting to invoke validateDevices while recording audio");
        return;
    }

    if (m_recordDevice.has_value()) {
        // check if this device still exists
        if (auto dev = this->getRecordingDevice(m_recordDevice->id)) {
            m_recordDevice = dev;
        } else {
            log::warn("Invalidating audio device {} ({})", m_recordDevice->id, m_recordDevice->name);
            m_recordDevice = std::nullopt;
        }
    }
}

void AudioManager::refreshDevices() {
    this->validateDevices();

    if (!m_recordDevice) {
        auto devices = this->getRecordingDevices();
        if (!devices.empty()) {
            m_recordDevice = devices[0];
            log::info("Selected new audio device {} ({})", m_recordDevice->id, m_recordDevice->name);
        }
    }
}

void AudioManager::setRecordBufferCapacity(size_t frames) {
    m_recordFrame.setCapacity(frames);
}

Result<> AudioManager::startRecordingEncoded(
    std23::move_only_function<void(const EncodedAudioFrame&)>&& encodedCallback
) {
    GEODE_UNWRAP(this->startRecordingInternal());
    m_callback = std::move(encodedCallback);
    m_rawCallback = nullptr;
    m_recordingRaw = false;
    return Ok();
}

Result<> AudioManager::startRecordingRaw(
    std23::move_only_function<void(const float*, size_t)>&& rawCallback
) {
    GEODE_UNWRAP(this->startRecordingInternal());
    m_rawCallback = std::move(rawCallback);
    m_callback = nullptr;
    m_recordingRaw = true;
    return Ok();
}

Result<> AudioManager::startRecordingInternal() {
    if (!permission::getPermissionStatus(Permission::RecordAudio)) {
        return Err("Recording failed, please grant microphone permission in Globed settings");
    }

    if (!m_recordDevice) {
        return Err("No recording device selected");
    }

    if (this->isRecording() || m_recordActive) {
        return Err("Already recording");
    }

    if (m_recordSound) {
        m_recordSound->release();
        m_recordSound = nullptr;
    }

    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.length = sizeof(float) * exinfo.defaultfrequency * exinfo.numchannels;
    m_recordChunkSize = exinfo.length;

    FMOD_ERRC(
        this->getSystem()->createSound(
            nullptr,
            FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL | FMOD_CREATESAMPLE,
            &exinfo,
            &m_recordSound
        ),
        "FMOD::System::createSound"
    );

    FMOD_RESULT res = this->getSystem()->recordStart(m_recordDevice->id, m_recordSound, true);

    // invalid device most likely
    if (res == FMOD_ERR_RECORD) {
        return Err("Failed to start recording audio");
    }

    FMOD_ERRC(res, "FMOD::System::recordStart");

    m_recordQueuedStop = false;
    m_recordQueuedHalt = false;
    m_recordLastPosition = 0;
    m_recordActive = true;

    return Ok();
}

void AudioManager::stopRecording() {
    m_recordQueuedStop = true;
}

void AudioManager::haltRecording() {
    m_recordQueuedHalt = true;
    m_recordQueuedStop = true;
}

bool AudioManager::isRecording() {
    if (!m_recordDevice) {
        return false;
    }

    if (m_recordActive && m_recordingPassive) {
        return m_recordingPassiveActive;
    }

    bool recording;

    if (FMOD_OK != this->getSystem()->isRecording(m_recordDevice->id, &recording)) {
        m_recordDevice = std::nullopt;
        this->stopRecording();
        return false;
    }

    return recording && m_recordActive;
}

void AudioManager::setPassiveMode(bool passive) {
    m_recordingPassive = passive;
}

void AudioManager::resumePassiveRecording() {
    m_recordingPassiveActive = true;
}

void AudioManager::pausePassiveRecording() {
    m_recordingPassiveActive = false;
}

Result<FMOD::Channel*> AudioManager::playSound(FMOD::Sound* sound) {
    FMOD::Channel* ch = nullptr;

    FMOD_ERRC(
        this->getSystem()->playSound(sound, nullptr, false, &ch),
        "System::playSound"
    );

    return Ok(ch);
}

Result<FMOD::Sound*> AudioManager::createSound(const float* pcm, size_t samples, int sampleRate) {
    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = sampleRate;
    exinfo.length = sizeof(float) * samples;

    FMOD::Sound* sound;

    FMOD_ERRC(this->getSystem()->createSound(
        nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &sound),
        "System::createSound"
    );

    float* data;
    FMOD_ERRC(
        sound->lock(0, exinfo.length, (void**)&data, nullptr, nullptr, nullptr),
        "Sound::lock"
    );

    std::memcpy(data, pcm, exinfo.length);

    FMOD_ERRC(
        sound->unlock(data, nullptr, exinfo.length, 0),
        "Sound::unlock"
    );

    return Ok(sound);
}

void AudioManager::setActiveRecordingDevice(int deviceId) {
    auto dev = this->getRecordingDevice(deviceId);

    if (dev.has_value()) {
        this->setActiveRecordingDevice(dev.value());
    }
}

void AudioManager::setActiveRecordingDevice(const AudioRecordingDevice& device) {
    if (this->isRecording()) {
        globed::toastError("Attempting to change recording device while recording audio");
        return;
    }

    m_recordDevice = device;
}

void AudioManager::toggleLoopbacksAllowed(bool allowed) {
    m_loopbacksAllowed = allowed;
}

FMOD::System* AudioManager::getSystem() {
    if (!m_system) {
        m_system = FMODAudioEngine::get()->m_system;
    }

    return m_system;
}

void AudioManager::forEachStream(std23::function_ref<void(int, AudioStream&)> func) {
    for (auto& [id, stream] : m_playbackStreams) {
        func(id, *stream);
    }
}

AudioStream* AudioManager::getStream(int streamId) {
    auto it = m_playbackStreams.find(streamId);
    if (it == m_playbackStreams.end()) {
        return nullptr;
    }

    return it->second.get();
}

Result<> AudioManager::playFrameStreamed(int streamId, const EncodedAudioFrame& frame) {
    auto stream = this->preparePlaybackStream(streamId);

    return stream->writeData(frame);
}

void AudioManager::playFrameStreamedRaw(int streamId, const float* pcm, size_t samples) {
    auto stream = this->preparePlaybackStream(streamId);

    stream->writeData(pcm, samples);
}

AudioStream* AudioManager::preparePlaybackStream(int id) {
    auto it = m_playbackStreams.find(id);

    if (it != m_playbackStreams.end()) {
        return it->second.get();
    }

    auto stream = std::make_unique<AudioStream>(AudioDecoder{
        VOICE_TARGET_SAMPLERATE,
        VOICE_TARGET_FRAMESIZE,
        VOICE_CHANNELS,
    });
    stream->start();

    auto ptr = stream.get();
    m_playbackStreams[id] = std::move(stream);

    return ptr;
}

void AudioManager::stopAllOutputStreams() {
    m_playbackStreams.clear();
}

void AudioManager::stopOutputStream(int streamId) {
    m_playbackStreams.erase(streamId);
}

float AudioManager::getStreamVolume(int streamId) {
    auto it = m_playbackStreams.find(streamId);
    if (it == m_playbackStreams.end()) {
        return 0.0f;
    }

    return it->second->getUserVolume();
}

float AudioManager::getStreamLoudness(int streamId) {
    auto it = m_playbackStreams.find(streamId);
    if (it == m_playbackStreams.end()) {
        return 0.0f;
    }
    return it->second->getLoudness();
}

void AudioManager::setStreamVolume(int streamId, float volume) {
    auto it = m_playbackStreams.find(streamId);

    if (it != m_playbackStreams.end()) {
        float vol = this->mapStreamVolume(streamId, volume);
        it->second->setUserVolume(vol, m_globalPlaybackLayer);
    }
}

void AudioManager::setGlobalPlaybackVolume(float volume) {
    m_playbackLayer = volume;
    this->updatePlaybackVolume();
}

bool AudioManager::isStreamActive(int streamId) {
    auto it = m_playbackStreams.find(streamId);
    return it != m_playbackStreams.end() && !it->second->isStarving();
}

void AudioManager::setDeafen(bool deafen) {
    m_deafen = deafen;
    this->updatePlaybackVolume();
}

bool AudioManager::getDeafen() {
    return m_deafen;
}

void AudioManager::setFocusedPlayer(int playerId) {
    m_focusedPlayer = playerId;
    this->updatePlaybackVolume();
}

void AudioManager::clearFocusedPlayer() {
    this->setFocusedPlayer(-1);
}

int AudioManager::getFocusedPlayer() {
    return m_focusedPlayer;
}

float AudioManager::mapStreamVolume(int playerId, float vol) {
    if (m_focusedPlayer == -1) {
        return vol;
    } else if (m_focusedPlayer == playerId) {
        return vol;
    } else {
        return vol * 0.2f;
    }
}

void AudioManager::updatePlaybackVolume() {
    m_globalPlaybackLayer = m_playbackLayer * (m_deafen ? 0.f : 1.f);

    for (auto& [id, stream] : m_playbackStreams) {
        float vol = this->mapStreamVolume(id, stream->getUserVolume());
        stream->setUserVolume(vol, m_globalPlaybackLayer);
    }
}

// Thread stuff

void AudioManager::audioThreadFunc(decltype(m_thread)::StopToken&) {
    // if we are not recording right now, sleep
    if (!m_recordActive) {
        m_thrSleeping = true;
        asp::time::sleep(asp::time::Duration::fromMillis(5));
        return;
    }

    // if someone queued us to stop recording, back to sleeping
    if (m_recordQueuedStop) {
        m_recordQueuedStop = true;
        m_thrSleeping = true;
        this->internalStopRecording();
        return;
    }

    m_thrSleeping = false;

    auto result = this->audioThreadWork();

    if (!result) {
        globed::toastError("{}", result.unwrapErr());
        m_thrSleeping = true;
        this->internalStopRecording();
    }
}

void AudioManager::internalStopRecording(bool ignoreErrors) {
    auto res =  this->getSystem()->recordStop(m_recordDevice->id);

    if (res != FMOD_OK && !ignoreErrors) {
        globed::toastError("{}", formatFmodError(res, "FMOD::System::recordStop"));
    }

    // if halting instead of stopping, don't call the callback
    if (m_recordQueuedHalt) {
        m_recordFrame.clear();
        m_recordQueuedHalt = false;
    } else {
        // call the callback if there's any audio leftover
        this->recordInvokeCallback();
    }

    // cleanup
    m_callback = [](const auto&){};
    m_rawCallback = [](const auto*, auto) {};
    m_recordLastPosition = 0;
    m_recordChunkSize = 0;
    m_recordingRaw = false;
    m_recordingPassive = false;
    m_recordingPassiveActive = false;
    m_recordQueue.clear();

    if (m_recordSound) {
        m_recordSound->release();
        m_recordSound = nullptr;
    }

    m_recordActive = false;
    m_recordQueuedStop = false;
}

Result<> AudioManager::audioThreadWork() {
    float* pcmData;
    unsigned int pcmLen;

    unsigned int pos;
    FMOD_ERRC(
        this->getSystem()->getRecordPosition(m_recordDevice->id, &pos),
        "System::getRecordPosition"
    );

    // if we are at the same position, do nothing
    if (pos == m_recordLastPosition) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return Ok();
    }

    FMOD_ERRC(
        m_recordSound->lock(0, m_recordChunkSize, (void**)&pcmData, nullptr, &pcmLen, nullptr),
        "Sound::lock"
    );

    // don't write any data if we are in passive recording and not currently recording
    if (!m_recordingPassive || m_recordingPassiveActive) {
        if (pos > m_recordLastPosition) {
            m_recordQueue.writeData(pcmData + m_recordLastPosition, pos - m_recordLastPosition);
        } else if (pos < m_recordLastPosition) { // we reached the end of the buffer
            // write the data left at the end
            m_recordQueue.writeData(pcmData + m_recordLastPosition, pcmLen / sizeof(float) - m_recordLastPosition);

            // write the data at the start of the buffer
            m_recordQueue.writeData(pcmData, pos);
        }
    }

    m_recordLastPosition = pos;

    FMOD_ERRC(
        m_recordSound->unlock(pcmData, nullptr, pcmLen, 0),
        "Sound::unlock"
    );

    if (m_recordingRaw) {
        size_t samples = m_recordQueue.size();

        // raw recording, call the raw callback with the pcm data directly.
        if (auto pcm = m_recordQueue.contiguousData()) {
            this->recordInvokeRawCallback(*pcm, samples);
        } else {
            auto pcmdata = std::make_unique<float[]>(samples);
            m_recordQueue.readData(pcmdata.get(), samples);
            this->recordInvokeRawCallback(pcmdata.get(), samples);
        }

        m_recordQueue.clear();
    } else {
        bool stoppedPassive = m_recordingPassive && !m_recordingPassiveActive;
        // encoded recording, encode the data and push to the frame.
        if (m_recordQueue.size() >= VOICE_TARGET_FRAMESIZE || stoppedPassive) {
            float pcmbuf[VOICE_TARGET_FRAMESIZE];
            size_t filled = m_recordQueue.readData(pcmbuf, VOICE_TARGET_FRAMESIZE);

            if (filled < VOICE_TARGET_FRAMESIZE) {
                // simply discard data
            } else {
                GEODE_UNWRAP_INTO(auto opusFrame, m_encoder.encode(pcmbuf));
                GEODE_UNWRAP(m_recordFrame.pushOpusFrame(opusFrame));
            }
        }

        if (m_recordFrame.size() >= m_recordFrame.capacity() || (stoppedPassive && m_recordFrame.size() > 0)) {
            this->recordInvokeCallback();
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(4));

    return Ok();
}

void AudioManager::recordInvokeCallback() {
    if (m_recordFrame.size() == 0) return;

    if (m_callback) m_callback(m_recordFrame);
    m_recordFrame.clear();
}

void AudioManager::recordInvokeRawCallback(const float* pcm, size_t samples) {
    if (samples == 0) return;

    if (m_rawCallback) m_rawCallback(pcm, samples);
}

}

$on_mod(Loaded) {
#ifdef GLOBED_VOICE_SUPPORT
    globed::AudioManager::get().preInitialize();
#endif
}