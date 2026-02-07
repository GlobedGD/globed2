#include <globed/audio/AudioManager.hpp>
#include <globed/audio/sound/PlayerSound.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/util/format.hpp>

#ifdef GEODE_IS_WINDOWS
# include <objbase.h>
#endif

#include <fmod_errors.h>
#include <Geode/utils/permission.hpp>
#include <arc/time/Sleep.hpp>

using namespace geode::prelude;
using enum std::memory_order;
namespace permission = geode::utils::permission;
using permission::Permission;

static std::string formatFmodError(FMOD_RESULT result, const char* whatFailed) {
    return fmt::format("{} failed: [FMOD error {}] {}", whatFailed, (int)result, FMOD_ErrorString(result));
}

#define FMOD_ERRC(res, msg) \
    do { \
        auto _res = (res); \
        if (_res != FMOD_OK) return Err(formatFmodError(_res, msg)); \
    } while (0)

namespace globed {

static float proximityVolumeMult(float distance) {
    distance = std::clamp(distance, 0.0f, PROXIMITY_AUDIO_LIMIT);
    float t = distance / PROXIMITY_AUDIO_LIMIT;
    return 1.0f - t * t;
}

AudioManager::AudioManager()
    : m_encoder(VOICE_TARGET_SAMPLERATE, VOICE_TARGET_FRAMESIZE, VOICE_CHANNELS) {

    m_workerTask = async::spawn(this->threadFunc());
    m_workerTask->setName("[Globed] Audio worker");

    m_recordDevice = AudioRecordingDevice{.id = -1};

    m_vcVolume = SettingsManager::get().getAndListenForChanges<float>("core.audio.playback-volume", [this](float value) {
        m_vcVolume = value;
    });

    m_sfxVolume = SettingsManager::get().getAndListenForChanges<float>("core.player.quick-chat-sfx-volume", [this](float value) {
        m_sfxVolume = value;
    });
}

AudioManager::~AudioManager() {
    if (m_workerTask) {
        m_workerTask->abort();
        m_workerTask->blockOn();
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

    if (!m_loopbacksAllowed.load(relaxed) && (
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
    geode::Function<void(const EncodedAudioFrame&)>&& encodedCallback
) {
    GEODE_UNWRAP(this->startRecordingInternal());
    m_callback = std::move(encodedCallback);
    m_rawCallback = nullptr;
    m_recordingRaw.store(false, relaxed);
    return Ok();
}

Result<> AudioManager::startRecordingRaw(
    geode::Function<void(const float*, size_t)>&& rawCallback
) {
    GEODE_UNWRAP(this->startRecordingInternal());
    m_rawCallback = std::move(rawCallback);
    m_callback = nullptr;
    m_recordingRaw.store(true, relaxed);
    return Ok();
}

Result<> AudioManager::startRecordingInternal() {
    if (!permission::getPermissionStatus(Permission::RecordAudio)) {
        return Err("Recording failed, please grant microphone permission in Globed settings");
    }

    if (!m_recordDevice) {
        return Err("No recording device selected");
    }

    if (this->isRecording() || m_recordActive.load(relaxed)) {
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

    m_recordQueuedStop.store(false, relaxed);
    m_recordQueuedHalt.store(false, relaxed);
    m_recordLastPosition = 0;
    m_recordActive.store(true, relaxed);
    m_workerNotify.notifyOne(true);

    return Ok();
}

void AudioManager::stopRecording() {
    m_recordQueuedStop.store(true, relaxed);
}

void AudioManager::haltRecording() {
    m_recordQueuedHalt.store(true, relaxed);
    m_recordQueuedStop.store(true, relaxed);
}

bool AudioManager::isRecording() {
    if (!m_recordDevice) {
        return false;
    }

    if (m_recordActive.load(relaxed) && m_recordingPassive.load(relaxed)) {
        return m_recordingPassiveActive.load(relaxed);
    }

    bool recording;

    if (FMOD_OK != this->getSystem()->isRecording(m_recordDevice->id, &recording)) {
        m_recordDevice = std::nullopt;
        this->stopRecording();
        return false;
    }

    return recording && m_recordActive.load(relaxed);
}

void AudioManager::setPassiveMode(bool passive) {
    m_recordingPassive.store(passive, relaxed);
}

void AudioManager::resumePassiveRecording() {
    m_recordingPassiveActive.store(true, relaxed);
}

void AudioManager::pausePassiveRecording() {
    m_recordingPassiveActive.store(false, relaxed);
}

Result<FMOD::Channel*> AudioManager::playSound(FMOD::Sound* sound) {
    FMOD::Channel* ch = nullptr;

    FMOD_ERRC(
        this->getSystem()->playSound(sound, nullptr, false, &ch),
        "System::playSound"
    );

    return Ok(ch);
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
    m_loopbacksAllowed.store(allowed, relaxed);
}

FMOD::System* AudioManager::getSystem() {
    if (!m_system) {
        m_system = FMODAudioEngine::get()->m_system;
    }

    return m_system;
}

Result<> AudioManager::mapError(FMOD_RESULT result) {
    if (result == FMOD_OK) return Ok();

    return Err("[FMOD error {}] {}", (int)result, FMOD_ErrorString(result));
}

void AudioManager::stopAllOutputSources() {
    for (auto& src : m_playbackSources) {
        src->stop();
    }
    m_playbackSources.clear();
}

float AudioManager::calculateVolume(AudioSource& src, const CCPoint& playerPos, bool voiceProximity) {
    if (m_deafen) return 0.f;

    float targetVolume = src.getVolume();
    auto kind = src.kind();

    // proximity
    bool applyProximity = kind == AudioKind::VoiceChat
        ? voiceProximity
        : true;

    if (applyProximity) {
        auto pos = src.getPosition();
        if (pos) {
            auto distance = playerPos.getDistance(*pos);
            targetVolume *= proximityVolumeMult(distance);
        }
    }

    // kind multipliers
    switch (kind) {
        case AudioKind::EmoteSfx: {
            targetVolume *= m_sfxVolume;
        } break;

        case AudioKind::VoiceChat: {
            targetVolume *= m_vcVolume;
        } break;

        default: break;
    }

    // focused player
    if (m_focusedPlayer != -1) {
        int id = -1;

        if (auto ps = dynamic_cast<PlayerSound*>(&src)) {
            if (auto pl = ps->getPlayer()) {
                id = pl->id();
            }
        }

        if (m_focusedPlayer != id) {
            targetVolume *= 0.2f;
        }
    }

    return targetVolume;
}

void AudioManager::updatePlayback(CCPoint playerPos, bool voiceProximity) {
    for (auto it = m_playbackSources.begin(); it != m_playbackSources.end(); ) {
        auto& src = *it;
        if (!src->isPlaying()) {
            src->stop();
            it = m_playbackSources.erase(it);
            continue;
        }

        float tvol = this->calculateVolume(*src, playerPos, voiceProximity);
        src->rawSetVolume(tvol);
        src->onUpdate();

        ++it;
    }
}

void AudioManager::registerPlaybackSource(std::shared_ptr<AudioSource> source) {
    m_playbackSources.insert(source);
}

void AudioManager::setDeafen(bool deafen) {
    m_deafen = deafen;
}

bool AudioManager::getDeafen() {
    return m_deafen;
}

void AudioManager::setFocusedPlayer(int playerId) {
    m_focusedPlayer = playerId;
}

void AudioManager::clearFocusedPlayer() {
    this->setFocusedPlayer(-1);
}

int AudioManager::getFocusedPlayer() {
    return m_focusedPlayer;
}

// Thread stuff

arc::Future<> AudioManager::threadFunc() {
    auto notify = m_workerNotify;
    
    while (true) {
        // if we are not recording right now, wait for something to happen
        if (!m_recordActive.load(relaxed)) {
            m_thrSleeping.store(true, relaxed);
            co_await notify.notified();
            continue;
        }

        // if someone queued us to stop recording, back to sleeping
        if (m_recordQueuedStop.load(relaxed)) {
            m_thrSleeping.store(true, relaxed);
            this->internalStopRecording();
            co_return;
        }
        
        m_thrSleeping.store(false, relaxed);
        
        auto result = this->audioThreadWork();

        if (!result) {
            log::warn("audioThreadWork failed: {}", result.unwrapErr());

            geode::queueInMainThread([result = std::move(result)] {
                globed::toastError("{}", result.unwrapErr());
            });

            m_thrSleeping.store(true, relaxed);
            this->internalStopRecording();
        }

        co_await arc::sleep(asp::Duration::fromMillis(3));
    }
}

void AudioManager::internalStopRecording(bool ignoreErrors) {
    auto res =  this->getSystem()->recordStop(m_recordDevice->id);

    if (res != FMOD_OK && !ignoreErrors) {
        geode::queueInMainThread([res = std::move(res)] {
            globed::toastError("{}", formatFmodError(res, "FMOD::System::recordStop"));
        });
    }

    // if halting instead of stopping, don't call the callback
    if (m_recordQueuedHalt.exchange(false, relaxed)) {
        m_recordFrame.clear();
    } else {
        // call the callback if there's any audio leftover
        this->recordInvokeCallback();
    }

    // cleanup
    m_callback = [](const auto&){};
    m_rawCallback = [](const auto*, auto) {};
    m_recordLastPosition = 0;
    m_recordChunkSize = 0;
    m_recordingRaw.store(false, relaxed);
    m_recordingPassive.store(false, relaxed);
    m_recordingPassiveActive.store(false, relaxed);
    m_recordQueue.clear();

    if (m_recordSound) {
        m_recordSound->release();
        m_recordSound = nullptr;
    }

    m_recordActive.store(false, relaxed);
    m_recordQueuedStop.store(false, relaxed);
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
        return Ok();
    }

    FMOD_ERRC(
        m_recordSound->lock(0, m_recordChunkSize, (void**)&pcmData, nullptr, &pcmLen, nullptr),
        "Sound::lock"
    );

    // don't write any data if we are in passive recording and not currently recording
    if (!m_recordingPassive.load(relaxed) || m_recordingPassiveActive.load(relaxed)) {
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

    if (m_recordingRaw.load(relaxed)) {
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
        bool stoppedPassive = m_recordingPassive.load(relaxed) && !m_recordingPassiveActive.load(relaxed);
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