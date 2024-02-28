#pragma once
#include <defs/platform.hpp>
#include <defs/util.hpp>

#include "manager.hpp"

class VoiceRecordingManager : public SingletonBase<VoiceRecordingManager> {
protected:
    VoiceRecordingManager();
    friend class SingletonBase;

public:
#if GLOBED_VOICE_SUPPORT
    util::sync::SmartThread<VoiceRecordingManager*> thread;
    util::sync::AtomicBool queuedStop = false, queuedStart = false, recording = false;

    void threadFunc();
#endif // GLOBED_VOICE_SUPPORT

    void startRecording();
    void stopRecording();
    bool isRecording();

private:
    void resetBools(bool recording);
};
