#include "voice_record_manager.hpp"

#ifdef GLOBED_VOICE_SUPPORT

#include <data/packets/client/misc.hpp>
#include <data/packets/client/game.hpp>
#include <managers/error_queues.hpp>
#include <audio/manager.hpp>
#include <net/manager.hpp>
#include <util/time.hpp>

VoiceRecordingManager::VoiceRecordingManager() {
    thread.setStartFunction([] { geode::utils::thread::setName("Record Thread"); });
    thread.setLoopFunction(&VoiceRecordingManager::threadFunc);
    thread.start(this);
}

void VoiceRecordingManager::startRecording() {
    queuedStart = true;
}

void VoiceRecordingManager::stopRecording() {
    queuedStop = true;
}

void VoiceRecordingManager::threadFunc() {
    auto& vm = GlobedAudioManager::get();

    if (queuedStop) {
        if (vm.isRecording()) {
            vm.stopRecording();
        }

        this->resetBools(false);
        return;
    }

    if (queuedStart) {
        if (!vm.isRecording()) {
            vm.validateDevices();

            // make sure the recording device is valid
            if (!vm.isRecordingDeviceSet()) {
                ErrorQueues::get().debugWarn("Unable to record audio, no recording device is set");
                this->resetBools(false);
                return;
            }

            auto result = vm.startPassiveRecording([](const auto& frame) {
                auto& nm = NetworkManager::get();
                if (!nm.established()) return;

                // `frame` does not live long enough and will be destructed at the end of this callback.
                // so we can't pass it directly in a `VoicePacket` and we use a `RawPacket` instead.

                ByteBuffer buf;
                buf.writeValue(frame);

                nm.send(RawPacket::create<VoicePacket>(std::move(buf)));
            });

            if (result.isErr()) {
                ErrorQueues::get().warn(result.unwrapErr());
                log::warn("unable to record audio: {}", result.unwrapErr());
                this->resetBools(false);
                return;
            }
        }
    }

    this->resetBools(vm.isRecording());

    std::this_thread::sleep_for(util::time::millis(10));
}

void VoiceRecordingManager::resetBools(bool recording) {
    this->recording = recording;
    queuedStart = false;
    queuedStop = false;
}

bool VoiceRecordingManager::isRecording() {
    return recording;
}

#else

VoiceRecordingManager::VoiceRecordingManager() {}
void VoiceRecordingManager::startRecording() {}
void VoiceRecordingManager::stopRecording() {}
void VoiceRecordingManager::resetBools(bool recording) {}
bool VoiceRecordingManager::isRecording() {
    return false;
}

#endif // GLOBED_VOICE_SUPPORT