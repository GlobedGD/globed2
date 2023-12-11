#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include <thread>
#include <functional>
#include <fmod.hpp>

#include "frame.hpp"
#include "sample_queue.hpp"
#include <util/sync.hpp>

using util::sync::WrappingMutex;
using util::sync::AtomicBool;

struct AudioRecordingDevice {
    int id = -1;
    std::string name;
    FMOD_GUID guid;
    int sampleRate;
    FMOD_SPEAKERMODE speakerMode;
    int speakerModeChannels;
    FMOD_DRIVER_STATE driverState;
};

struct AudioPlaybackDevice {
    int id = -1;
    std::string name;
    FMOD_GUID guid;
    int sampleRate;
    FMOD_SPEAKERMODE speakerMode;
    int speakerModeChannels;
};

constexpr size_t VOICE_TARGET_SAMPLERATE = 24000;
constexpr float VOICE_CHUNK_RECORD_TIME = 0.06f; // the audio buffer that is recorded at once (60ms)
constexpr size_t VOICE_TARGET_FRAMESIZE = VOICE_TARGET_SAMPLERATE * VOICE_CHUNK_RECORD_TIME; // opus framesize
constexpr size_t VOICE_CHANNELS = 1;

// This class is not thread safe. At all.
class GlobedAudioManager : GLOBED_SINGLETON(GlobedAudioManager) {
public:
    GlobedAudioManager();
    ~GlobedAudioManager();

    std::vector<AudioRecordingDevice> getRecordingDevices();
    std::vector<AudioPlaybackDevice> getPlaybackDevices();

    // get the record device by device ID
    std::optional<AudioRecordingDevice> getRecordingDevice(int deviceId);
    // get the playback device by device ID
    std::optional<AudioPlaybackDevice> getPlaybackDevice(int deviceId);

    // get the current active record device
    AudioRecordingDevice getRecordingDevice();
    // get the current active playback device
    AudioPlaybackDevice getPlaybackDevice();

    // get if the recording device is set
    bool isRecordingDeviceSet();

    // if the current selected recording/playback is invalid (i.e. disconnected),
    // it will be reset. if no device is selected or a valid device is selected, nothing happens.
    void validateDevices();

    // start recording the voice and call the callback once a full frame is ready.
    // if `stopRecording()` is called at any point, the callback will be called with the remaining data.
    // in that case it may have less than the full 10 frames.
    // WARNING: the callback is called from the audio thread, not the GD/cocos thread.
    void startRecording(std::function<void(const EncodedAudioFrame&)> callback);
    // tell the audio thread to stop recording
    void stopRecording();
    // tell the audio thread to stop recording, don't call the callback with leftover data
    void haltRecording();
    bool isRecording();

    // play a sound and return the channel associated with it
    [[nodiscard]] FMOD::Channel* playSound(FMOD::Sound* sound);

    // create a sound from raw PCM data
    [[nodiscard]] FMOD::Sound* createSound(const float* pcm, size_t samples, int sampleRate = VOICE_TARGET_SAMPLERATE);

    void setActiveRecordingDevice(int deviceId);
    void setActivePlaybackDevice(int deviceId);

    // get the cached system
    FMOD::System* getSystem();

    static const char* fmodErrorString(FMOD_RESULT result);
    static std::string formatFmodError(FMOD_RESULT result, const char* whatFailed);

private:
    /* devices */
    AudioRecordingDevice recordDevice;
    AudioPlaybackDevice playbackDevice; // unused

    /* recording */
    AtomicBool recordActive = false;
    AtomicBool recordQueuedStop = false;
    AtomicBool recordQueuedHalt = false;
    FMOD::Sound* recordSound = nullptr;
    size_t recordChunkSize = 0;
    std::function<void(const EncodedAudioFrame&)> recordCallback;
    AudioSampleQueue recordQueue;
    unsigned int recordLastPosition = 0;
    EncodedAudioFrame recordFrame;

    void recordContinueStream();
    void recordInvokeCallback();
    void internalStopRecording();

    AudioEncoder encoder;

    /* misc */
    AtomicBool _terminating = false;
    FMOD::System* cachedSystem = nullptr;

    void audioThreadFunc();
    AtomicBool audioThreadSleeping = true;
    std::thread audioThreadHandle;
};

#endif // GLOBED_VOICE_SUPPORT
