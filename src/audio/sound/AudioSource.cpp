#include <globed/audio/sound/AudioSource.hpp>
#include <globed/audio/AudioManager.hpp>

namespace globed {

void AudioSource::registerSelf(std::shared_ptr<AudioSource> self) {
    AudioManager::get().registerPlaybackSource(self);
}

}