#include "player_list_popup.hpp"
#include <UIBuilder.hpp>

#include "player_list_cell.hpp"
#include <data/packets/all.hpp>
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>

using namespace geode::prelude;

bool PlayerListPopup::setup() {
    auto& nm = NetworkManager::get();
    if (!nm.handshaken()) {
        return false;
    }

    nm.addListener<PlayerListPacket>([this](PlayerListPacket* packet) {
        this->playerList = packet->data;
        this->onLoaded();
    });

    nm.send(RequestPlayerListPacket::create());

    this->setTitle("Online players");

    // show an error popup if no response after 2 seconds
    timeoutSequence = CCSequence::create(
        CCDelayTime::create(2.0f),
        CCCallFunc::create(this, callfunc_selector(PlayerListPopup::onLoadTimeout)),
        nullptr
    );

    this->runAction(timeoutSequence);

    auto listview = ListView::create(CCArray::create(), PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT);
    listLayer = GJCommentListLayer::create(listview, "", {192, 114, 62, 255}, LIST_WIDTH, LIST_HEIGHT, false);

    float xpos = (m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2;
    listLayer->setPosition({xpos, 50.f});
    m_mainLayer->addChild(listLayer);

    loadingCircle = LoadingCircle::create();
    loadingCircle->setParentLayer(listLayer);
    loadingCircle->setPosition(-listLayer->getPosition());
    loadingCircle->show();

    return true;
}

void PlayerListPopup::onLoaded() {
    if (timeoutSequence) {
        this->stopAction(timeoutSequence);
        timeoutSequence = nullptr;
    }

    this->removeLoadingCircle();

    auto cells = CCArray::create();

    for (const auto& pdata : playerList) {
        auto* cell = PlayerListCell::create(pdata);
        cells->addObject(cell);
    }

    listLayer->m_list->removeFromParent();
    listLayer->m_list = Build<ListView>::create(cells, PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();
}

void PlayerListPopup::onLoadTimeout() {
    ErrorQueues::get().error("Failed to fetch the player list! This is most likely a server-side bug or a problem with your connection!");
    this->removeLoadingCircle();
}

void PlayerListPopup::removeLoadingCircle() {
    if (loadingCircle) {
        loadingCircle->fadeAndRemove();
        loadingCircle = nullptr;
    }
}

PlayerListPopup::~PlayerListPopup() {
    NetworkManager::get().removeListener<PlayerListPacket>();
}

PlayerListPopup* PlayerListPopup::create() {
    auto ret = new PlayerListPopup;
    if (ret && ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}