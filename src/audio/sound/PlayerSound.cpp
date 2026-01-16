#include <globed/audio/sound/PlayerSound.hpp>
#include <globed/core/game/RemotePlayer.hpp>

using namespace geode::prelude;

namespace globed {

PlayerSound::PlayerSound(FMOD::Sound *sound, std::weak_ptr<RemotePlayer> player, CCPoint pos)
    : Sound(sound), m_player(player), m_position(pos)
{
}

Result<std::shared_ptr<PlayerSound>> PlayerSound::create(const std::filesystem::path &path,
                                                         std::shared_ptr<RemotePlayer> player, bool paused)
{
    return create(utils::string::pathToString(path).c_str(), player, paused);
}

Result<std::shared_ptr<PlayerSound>> PlayerSound::create(const char *path, std::shared_ptr<RemotePlayer> player,
                                                         bool paused)
{
    auto s = std::make_shared<PlayerSound>(GEODE_UNWRAP(createRaw(path)), player,
                                           player ? player->player1()->getLastPosition() : CCPoint{});

    // pre emptively register the sound so stop() gets called
    s->registerSelf(s);

    GEODE_UNWRAP(s->play(paused));
    return Ok(s);
}

void PlayerSound::setStatic(bool isStatic)
{
    m_static = isStatic;
}

void PlayerSound::setGlobal(bool isGlobal)
{
    m_global = isGlobal;
}

std::shared_ptr<RemotePlayer> PlayerSound::getPlayer() const
{
    return m_player.lock();
}

std::optional<CCPoint> PlayerSound::getPosition() const
{
    if (m_global)
        return std::nullopt;
    if (m_static)
        return m_position;

    if (auto player = m_player.lock()) {
        // if the player is too far and culled, return a position far away so the sound isn't played
        if (player->isPlayer1Culled()) {
            return CCPoint{-99999.f, -99999.f};
        }

        return player->player1()->getLastPosition();
    }

    return std::nullopt;
}

bool PlayerSound::isPlaying() const
{
    if (!Sound::isPlaying())
        return false;

    return !m_player.expired();
}

} // namespace globed