#pragma once
#include <defs.hpp>

#include "manager.hpp"

class VoiceRecordingManager : public SingletonBase<VoiceRecordingManager> {
protected:
    VoiceRecordingManager();
    friend class SingletonBase;

public:
    util::sync::SmartThread<VoiceRecordingManager*> thread;
    util::sync::AtomicBool queuedStop = false, queuedStart = false, recording = false;

    void threadFunc();

    void startRecording();
    void stopRecording();

private:
    void resetBools(bool recording);
};
