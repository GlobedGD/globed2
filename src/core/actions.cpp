#include <globed/core/actions.hpp>
#include <globed/util/singleton.hpp>

using namespace geode::prelude;

namespace globed {

void warpToSession(SessionId session) {}
void warpToLevel(int level) {}

void openUserProfile(int accountId, int userId, std::string_view username) {
    bool myself = accountId == cachedSingleton<GJAccountManager>()->m_accountID;

    if (!myself) {
        cachedSingleton<GameLevelManager>()->storeUserName(userId, accountId, gd::string(username));
    }

    ProfilePage::create(accountId, myself)->show();
}

void openUserProfile(const RoomPlayer& player) {
    openUserProfile(player.accountData);
}

void openUserProfile(const PlayerAccountData& player) {
    openUserProfile(player.accountId, player.userId, player.username);
}

}