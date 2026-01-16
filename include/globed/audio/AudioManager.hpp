#pragma once

#include "AudioSampleQueue.hpp"
#include "EncodedAudioFrame.hpp"
#include "sound/AudioSource.hpp"
#include <globed/prelude.hpp>

#include <asp/sync.hpp>
#include <asp/thread.hpp>
#include <fmod.hpp>

namespace globed {

constexpr size_t VOICE_TARGET_SAMPLERATE = 24000;
constexpr float VOICE_CHUNK_RECORD_TIME = 0.06f; // the audio buffer that is recorded at once (60ms)
constexpr size_t VOICE_TARGET_FRAMESIZE = VOICE_TARGET_SAMPLERATE * VOICE_CHUNK_RECORD_TIME; // opus framesize
constexpr size_t VOICE_CHANNELS = 1;

// how many units before the audio disappears
constexpr float PROXIMITY_AUDIO_LIMIT = 1200.f;

struct AudioRecordingDevice {
    int id = -1;
    std::string name;
    FMOD_GUID guid;
    int sampleRate;
    FMOD_SPEAKERMODE speakerMode;
    int speakerModeChannels;
    FMOD_DRIVER_STATE driverState;
};

class GLOBED_DLL AudioManager : public SingletonBase<AudioManager> {
protected:
    friend class SingletonBase;
    AudioManager();
    ~AudioManager();

public:
    // preinitialization, for more info open the implementation
    void preInitialize();

    /* Device API */

    std::vector<AudioRecordingDevice> getRecordingDevices();

    // get the record device by device ID
    std::optional<AudioRecordingDevice> getRecordingDevice(int deviceId);

    // get the current active record device
    const std::optional<AudioRecordingDevice> &getRecordingDevice();

    // get if the recording device is set
    bool isRecordingDeviceSet();

    // if the current selected recording/playback is invalid (i.e. disconnected),
    // it will be reset. if no device is selected or a valid device is selected, nothing happens.
    void validateDevices();
    void refreshDevices();

    /* Recording API */

    // set the amount of record frames in a buffer (used by the lowerAudioLatency setting)
    void setRecordBufferCapacity(size_t frames);

    // start recording the voice and call the callback whenever new data is ready.
    // same rules apply as with `startRecording`, except the callback includes raw PCM samples,
    // and is called much more often.
    Result<> startRecordingEncoded(std23::move_only_function<void(const EncodedAudioFrame &)> &&encodedCallback);
    Result<> startRecordingRaw(std23::move_only_function<void(const float *, size_t)> &&rawCallback);
    // tell the audio thread to stop recording
    void stopRecording();
    // tell the audio thread to stop recording, don't call the callback with leftover data
    void haltRecording();
    bool isRecording();

    /* Background recording API */
    void setPassiveMode(bool passive);

    void resumePassiveRecording();
    void pausePassiveRecording();
    inline bool isPassiveRecording()
    {
        return m_recordingPassiveActive;
    }

    /* Playback API */

    void stopAllOutputSources();

    // Updates all playback sources, removing finished ones and updating volumes of proximity sources
    void updatePlayback(CCPoint playerPos, bool voiceProximity);
    void registerPlaybackSource(std::shared_ptr<AudioSource> source);

    void setDeafen(bool deafen);
    bool getDeafen();

    void setFocusedPlayer(int playerId);
    void clearFocusedPlayer();
    int getFocusedPlayer();

    /* Misc */

    // play a sound and return the channel associated with it
    Result<FMOD::Channel *> playSound(FMOD::Sound *sound);

    // create a sound from raw PCM data
    Result<FMOD::Sound *> createSound(const float *pcm, size_t samples, int sampleRate = VOICE_TARGET_SAMPLERATE);
    // create a sound from a filepath
    Result<FMOD::Sound *> createSound(const std::filesystem::path &path);

    void setActiveRecordingDevice(int deviceId);
    void setActiveRecordingDevice(const AudioRecordingDevice &device);

    void toggleLoopbacksAllowed(bool allowed);

    // get the cached system
    FMOD::System *getSystem();

    Result<> mapError(FMOD_RESULT result);

private:
    /* devices */
    std::optional<AudioRecordingDevice> m_recordDevice;
    asp::AtomicBool m_loopbacksAllowed = false;

    /* recording */
    asp::AtomicBool m_recordActive = false;
    asp::AtomicBool m_recordQueuedStop = false;
    asp::AtomicBool m_recordQueuedHalt = false;
    asp::AtomicBool m_recordingRaw = false;
    asp::AtomicBool m_recordingPassive = false;
    asp::AtomicBool m_recordingPassiveActive = false;
    FMOD::Sound *m_recordSound = nullptr;
    size_t m_recordChunkSize = 0;
    std23::move_only_function<void(const EncodedAudioFrame &)> m_callback;
    std23::move_only_function<void(const float *, size_t)> m_rawCallback;
    AudioSampleQueue m_recordQueue;
    unsigned int m_recordLastPosition = 0;
    EncodedAudioFrame m_recordFrame;
    AudioEncoder m_encoder;
    FMOD::System *m_system = nullptr;

    /* thread */
    asp::AtomicBool m_thrSleeping = true;
    asp::Thread<AudioManager *> m_thread;

    void audioThreadFunc(decltype(m_thread)::StopToken &);
    Result<> audioThreadWork();

    Result<> startRecordingInternal();
    void internalStopRecording(bool ignoreErrors = false);
    void recordInvokeCallback();
    void recordInvokeRawCallback(const float *pcm, size_t samples);
    float calculateVolume(AudioSource &src, const CCPoint &playerPos, bool voiceProximity);

    /* playback */
    std::unordered_set<std::shared_ptr<AudioSource>> m_playbackSources;
    float m_vcVolume = 1.f;
    float m_sfxVolume = 1.f;
    int m_focusedPlayer = -1;
    bool m_deafen = false;
};

} // namespace globed