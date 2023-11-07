#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include <thread>
#include <functional>
#include <fmod.hpp>
#include <util/sync.hpp>
#include "opus_codec.hpp"
#include "audio_frame.hpp"

using util::sync::WrappingMutex;

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

class GlobedAudioManager {
public:
    GLOBED_SINGLETON(GlobedAudioManager);
    GlobedAudioManager();
    ~GlobedAudioManager();

    std::vector<AudioRecordingDevice> getRecordingDevices();
    std::vector<AudioPlaybackDevice> getPlaybackDevices();

    // get the record device by device ID
    AudioRecordingDevice getRecordingDevice(int deviceId);
    // get the playback device by device ID
    AudioPlaybackDevice getPlaybackDevice(int deviceId);

    // get the current active record device
    AudioRecordingDevice getRecordingDevice();
    // get the current active playback device
    AudioPlaybackDevice getPlaybackDevice();

    // start recording the voice and call the callback
    void startRecording(std::function<void(const EncodedAudioFrame&)> callback);
    void stopRecording();
    // like stopRecording but is safe to call from the callback in startRecording
    void queueStopRecording();
    bool isRecording();

    // play a sound
    void playSound(FMOD::Sound* sound);

    // create a sound from raw PCM data
    [[nodiscard]] FMOD::Sound* createSound(const float* pcm, size_t samples, int sampleRate = VOICE_TARGET_SAMPLERATE);
    
    void setActiveRecordingDevice(int deviceId);
    void setActivePlaybackDevice(int deviceId);

    // Decode a sound from opus into PCM-float. Not recommended to use directly unless you know what you are doing.
    [[nodiscard]] DecodedOpusData decodeSound(util::data::byte* data, size_t length);
    // Decode a sound from opus into PCM-float. Not recommended to use directly unless you know what you are doing.
    [[nodiscard]] DecodedOpusData decodeSound(const EncodedOpusData& data);

    // get the cached system
    FMOD::System* getSystem();

private:
    /* recording*/
    AudioRecordingDevice recordDevice;
    FMOD::Sound* recordSound = nullptr;
    bool recordActive = false;
    size_t recordChunkSize = 0;
    std::function<void(const EncodedAudioFrame&)> recordCallback;
    std::mutex recordMutex;
    std::atomic_bool recordQueuedStop = false;

    void recordContinueStream();

    AudioPlaybackDevice playbackDevice;

    /* opus */
    OpusCodec opus;

    /* misc */
    std::atomic_bool _terminating;
    FMOD::System* cachedSystem = nullptr;

    void audioThreadFunc();
    std::atomic_bool audioThreadSleeping = true;
    std::thread audioThreadHandle;
};

#endif // GLOBED_VOICE_SUPPORT