#include "room.hpp"
#include <Geode/Geode.hpp>

#include <util/gd.hpp>

#include <room.hpp>

using namespace geode::prelude;

RoomManager::RoomManager() {
    this->setGlobal();
}

RoomInfo& RoomManager::getInfo() {
    return roomInfo;
}

uint32_t RoomManager::getId() {
    return roomInfo.id;
}

bool RoomManager::isOwner() {
    return this->isInRoom() && roomInfo.owner.accountId == GJAccountManager::get()->m_accountID;
}

bool RoomManager::isInGlobal() {
    return roomInfo.id == 0;
}

bool RoomManager::isInRoom() {
    return !isInGlobal();
}

void RoomManager::setInfo(const RoomInfo& info) {
    bool levelChanged = info.settings.levelId != roomInfo.settings.levelId;
    bool roomIdChanged = info.id != roomInfo.id;

    roomInfo = info;

    if (levelChanged) {
        roomLevel = nullptr;
        this->cancelLevelFetching();
        if (info.settings.levelId) {
            this->fetchRoomLevel(info.settings.levelId);
        }
    }

    Loader::get()->queueInMainThread([roomIdChanged] {
        auto data = globed::room::getRoomData();
        if (!data) {
            // now in global room or perhaps 
            if (roomIDChanged) globed::RoomLeaveEvent().post();
            return;
        }

        // now in another room or the same room but updated
        if (roomIdChanged) {
            globed::RoomLeaveEvent().post();
            globed::RoomJoinEvent(data.unwrap()).post();
        } else {
            globed::RoomUpdateEvent(data.unwrap()).post();
        }
    });
}

void RoomManager::setGlobal() {
    this->setInfo(RoomInfo {
        .id = 0,
        .owner = {},
        .settings = {}
    });
}

GJGameLevel* RoomManager::getRoomLevel() {
    return roomLevel;
}

/* Fetching levels (ew) */

namespace {
class DummyLevelFetchNode : public CCObject, public LevelManagerDelegate, public LevelDownloadDelegate {
public:
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) override {
        if (this->fetchCallback) {
            this->fetchCallback(Ok(p0));
        }
    }

    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) override {
        this->loadLevelsFinished(p0, p1, 0);
    }

    void loadLevelsFailed(char const* p0, int p1) override {
        if (this->fetchCallback) {
            this->fetchCallback(Err(p1));
        }
    }

    void loadLevelsFailed(char const* p0) override {
        this->loadLevelsFailed(p0, 0);
    }

    std::function<void(Result<CCArray*, int>)> fetchCallback;

    void levelDownloadFinished(GJGameLevel* level) override {
        if (this->downloadCallback) {
            this->downloadCallback(Ok(level));
        }
    }

    void levelDownloadFailed(int x) override {
        if (this->downloadCallback) {
            this->downloadCallback(Err(x));
        }
    }

    std::function<void(Result<GJGameLevel*, int>)> downloadCallback;
};
}

static DummyLevelFetchNode* fetchNode = new DummyLevelFetchNode();

void RoomManager::fetchRoomLevel(int levelId) {
    auto* glm = GameLevelManager::get();
    if (glm->m_levelManagerDelegate || glm->m_levelDownloadDelegate) return;

    // fetch the level
    fetchNode->fetchCallback = [this, glm](Result<CCArray*, int> level) {
        glm->m_levelManagerDelegate = nullptr;

        if (!level) {
            log::warn("Failed to fetch level data (id {}) for room: code {}", roomInfo.settings.levelId, level.unwrapErr());
            fetchNode->fetchCallback = {};
            return;
        }

        fetchNode->downloadCallback = [this, glm](Result<GJGameLevel*, int> level) {
            glm->m_levelDownloadDelegate = nullptr;

            if (level.isOk()) {
                this->onRoomLevelDownloaded(level.unwrap());
            } else {
                log::warn("Failed to download level id {} for room: code {}", roomInfo.settings.levelId, level.unwrapErr());
            }

            fetchNode->downloadCallback = {};
        };

        auto arr = level.unwrap();
        if (arr->count()) {
            glm->m_levelDownloadDelegate = fetchNode;
            glm->downloadLevel(static_cast<GJGameLevel*>(arr->objectAtIndex(0))->m_levelID, false);
        }

        fetchNode->fetchCallback = {};
    };

    glm->m_levelManagerDelegate = fetchNode;
    glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, std::to_string(levelId)));
}

void RoomManager::onRoomLevelDownloaded(GJGameLevel* level) {
    util::gd::reorderDownloadedLevel(level);

    if (level->m_levelID == roomInfo.settings.levelId) {
        this->roomLevel = level;
    }
}

void RoomManager::cancelLevelFetching() {
    auto* glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = nullptr;
    glm->m_levelDownloadDelegate = nullptr;
}
