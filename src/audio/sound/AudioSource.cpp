#include <globed/audio/AudioManager.hpp>
#include <globed/audio/sound/AudioSource.hpp>

namespace globed {

void AudioSource::registerSelf(std::shared_ptr<AudioSource> self)
{
    AudioManager::get().registerPlaybackSource(self);
}

} // namespace globed