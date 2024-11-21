#include "room_listing_popup.hpp"

#include "room_listing_cell.hpp"
#include "room_join_popup.hpp"
#include <data/packets/client/room.hpp>
#include <data/packets/server/room.hpp>
#include <managers/admin.hpp>
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool RoomListingPopup::setup() {
	this->setTitle("Public Rooms");
    this->setID("room-listing"_spr);

    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        return false;
    }

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<RoomList>::create(contentSize.width, contentSize.height, globed::color::Brown, RoomListingCell::CELL_HEIGHT)
        .anchorPoint(0.5f, 1.f)
        .pos(rlayout.fromTop(40.f))
        .zOrder(2)
        .parent(m_mainLayer)
        .store(listLayer)
    ;

    nm.addListener<RoomListPacket>(this, [this](std::shared_ptr<RoomListPacket> packet) {
        // fake testing data
        if (GlobedSettings::get().launchArgs().fakeData) {
            auto& rng = util::rng::Random::get();

            auto randomSettings = [&]() -> RoomSettings {
                RoomSettings s = {};
                s.fasterReset = false;
                if (rng.generate<bool>()) {
                    s.levelId = rng.generate<uint32_t>(1, 100000);
                }

                if (rng.genRatio(0.25f)) {
                    s.playerLimit = rng.generate<uint16_t>(1, 1000);
                }

                s.flags.collision = rng.generate<bool>();
                s.flags.deathlink = rng.generate<bool>();
                s.flags.publicInvites = rng.generate<bool>();
                // s.flags.twoPlayerMode = rng.generate<bool>();

                return s;
            };

            packet->rooms.clear();
            size_t lim = rng.generate<size_t>(50, 150);
            for (size_t i = 0; i < lim; i++) {
                RoomListingInfo room = {
                    rng.generate<uint32_t>(1, 100000),
                    rng.generate<uint16_t>(1, 1000),
                    PlayerPreviewAccountData::makeRandom(),
                    fmt::format("Room {}", i),
                    rng.genRatio(0.3f),
                    randomSettings()
                };

                packet->rooms.push_back(room);
            }
        }

        this->createCells(packet->rooms);
    });

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    Build<CCScale9Sprite>::create("square02_small.png")
        .id("background")
        .contentSize(contentSize)
        .opacity(75)
        .anchorPoint(0.5f, 1.f)
        .pos(listLayer->getPosition())
        .parent(m_mainLayer)
        .zOrder(1)
        .store(background);

    auto* menu = Build<CCMenu>::create()
        .id("buttons-menu")
        .pos(0.f, 0.f)
        .contentSize(contentSize)
        .parent(m_mainLayer)
        .collect();

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->onReload(nullptr);
        })
        .pos(rlayout.bottomRight + CCPoint{-3.f, 3.f})
        .id("reload-btn"_spr)
        .parent(menu);

    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            RoomJoinPopup::create()->show();
        })
        .pos(rlayout.bottomLeft + CCPoint{3.f, 3.f})
        .id("add-room-btn")
        .parent(menu);

    if (AdminManager::get().canModerate()) {
        Build<CCMenu>::create()
            .id("mod-actions-menu")
            .pos(rlayout.fromBottom(20.f))
            .layout(
                RowLayout::create()
                    ->setGap(8.f)
                    ->setAutoScale(false)
            )
            .contentSize(60.f, 0.f)
            .parent(m_mainLayer)
            .child(Build<CCLabelBMFont>::create("Mod", "bigFont.fnt").scale(0.4f))
            .child(
                Build(CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [this](auto btn) {
                    bool enabled = !btn->isToggled();
                    this->toggleModActions(enabled);
                }))
            )
            .updateLayout()
            .with([&](auto* menu) {
                util::ui::attachBackground(menu);
            })
            ;
    }

    nm.send(RequestRoomListPacket::create());

    return true;
}

void RoomListingPopup::onReload(CCObject* sender) {
    NetworkManager::get().send(RequestRoomListPacket::create());
}

void RoomListingPopup::createCells(std::vector<RoomListingInfo> rlpv) {
    listLayer->removeAllCells();

    for (const RoomListingInfo& rlp : rlpv) {
        listLayer->addCell(rlp, this);
    }

    listLayer->sort([](RoomListingCell* a, RoomListingCell* b) {
        return a->playerCount > b->playerCount;
    });

    listLayer->scrollToTop();

    this->toggleModActions(this->modActionsOn);
}

void RoomListingPopup::toggleModActions(bool enabled) {
    this->modActionsOn = enabled;

    for (auto cell : (*listLayer)) {
        cell->toggleModActions(enabled);
    }
}

void RoomListingPopup::close() {
    this->onClose(nullptr);
}

RoomListingPopup* RoomListingPopup::create() {
	auto ret = new RoomListingPopup();

	if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, "GJ_square01.png")) {
		ret->autorelease();
		return ret;
	}

    delete ret;
	return nullptr;
}