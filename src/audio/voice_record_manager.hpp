#pragma once
#include <defs/platform.hpp>
#include <defs/util.hpp>

#include "manager.hpp"

class VoiceRecordingManager : public SingletonBase<VoiceRecordingManager> {
protected:
    VoiceRecordingManager();
    friend class SingletonBase;

public:
#ifdef GLOBED_VOICE_SUPPORT
    asp::Thread<VoiceRecordingManager*> thread;
    asp::AtomicBool queuedStop = false, queuedStart = false, recording = false;

    void threadFunc();
#endif // GLOBED_VOICE_SUPPORT

    void startRecording();
    void stopRecording();
    bool isRecording();

private:
    void resetBools(bool recording);
};
