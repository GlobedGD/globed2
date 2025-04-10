#include "swag_connection_test_popup.hpp"

#include <net/manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;
using ConnectionState = NetworkManager::ConnectionState;

static ccColor3B GREEN_DOT_COLOR = { 34, 236, 85 };
static ccColor3B WHITE_DOT_COLOR = {255, 255, 255};
class SwagConnectionTestPopup::StatusCell : public CCNode {
public:
    static StatusCell* create(const char* name, float width) {
        auto ret = new StatusCell;
        if (ret->init(name, width)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

protected:
    CCLabelBMFont* qmark;
    CCSprite* xIcon;
    CCSprite* checkIcon;

    CCLabelBMFont* nameLabel;
    CCLabelBMFont* stateLabel;

    bool init(const char* name, float width) {
        this->setContentSize({width, 12.f});

        Build<CCLabelBMFont>::create("?", "bigFont.fnt")
            .color({0, 250, 20})
            .store(qmark);

        Build<CCSprite>::createSpriteName("GJ_deleteIcon_001.png")
            .store(xIcon);

        Build<CCSprite>::createSpriteName("GJ_completesIcon_001.png")
            .store(checkIcon);

        CCSize statusIconSize{12.f, 12.f};
        float myVertCenter = this->getContentHeight() / 2.f;

        auto setStatusIconData = [&](CCNode* thing) {
            thing->setPosition({8.f, myVertCenter});
            util::ui::rescaleToMatch(thing, statusIconSize);
            this->addChild(thing);
        };

        setStatusIconData(qmark);
        setStatusIconData(xIcon);
        setStatusIconData(checkIcon);

        Build<CCLabelBMFont>::create(name, "bigFont.fnt")
            .scale(0.5f)
            .anchorPoint(0.f, 0.5f)
            .pos(20.f, myVertCenter)
            .parent(this)
            .store(nameLabel);

        Build<CCLabelBMFont>::create("", "bigFont.fnt")
            .scale(0.45f)
            .anchorPoint(0.f, 0.5f)
            .pos(200.f, myVertCenter)
            .parent(this)
            .store(stateLabel);

        this->setUnknown();

        return true;
    }

public:
    void setUnknown() {
        this->qmark->setVisible(true);
        this->xIcon->setVisible(false);
        this->checkIcon->setVisible(false);

        this->stateLabel->setString("Unknown");
    }

    void setFailed() {
        this->qmark->setVisible(false);
        this->xIcon->setVisible(true);
        this->checkIcon->setVisible(false);

        this->stateLabel->setString("Failed");
    }

    void setBlocked() {
        this->qmark->setVisible(false);
        this->xIcon->setVisible(true);
        this->checkIcon->setVisible(false);

        this->stateLabel->setString("Blocked");
    }

    void setSuccessful() {
        this->qmark->setVisible(false);
        this->xIcon->setVisible(false);
        this->checkIcon->setVisible(true);

        this->stateLabel->setString("Connected");
    }
};

bool SwagConnectionTestPopup::setup() {
    auto& nm = NetworkManager::get();
    if (nm.getConnectionState() != ConnectionState::Disconnected) {
        nm.disconnect();
    }

    this->setTitle("Connection test");

    xboxStyle = true;

    this->createCommonUi();

    this->createXboxStyleUi();

    auto* engine = FMODAudioEngine::sharedEngine();
    engine->playEffect("bootanim.ogg"_spr, 1.f, 1.f, 1.f);

    this->runAction(CCSequence::create(
        CCDelayTime::create(5.5f),
        CallFuncExt::create([this] {
            this->startTesting();
        }),
        nullptr
    ));

    return true;
}

void SwagConnectionTestPopup::createCommonUi() {
    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);
}

void SwagConnectionTestPopup::createXboxStyleUi() {
    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    float itemsY = rlayout.bottom + 24.f;
    CCSize iconSize {32.f, 32.f};

    Build<CCSprite>::createSpriteName("conn-test-desktop-icon.png"_spr)
        .with([&](auto spr) {
            util::ui::rescaleToMatch(spr, iconSize);
        })
        .pos(rlayout.fromBottomLeft(24.f, itemsY))
        .parent(m_mainLayer)
        .store(consoleIcon);

    Build<CCSprite>::createSpriteName("conn-test-bars-icon.png"_spr)
        .color({255, 255, 255})
        .with([&](auto spr) {
            util::ui::rescaleToMatch(spr, iconSize);
        })
        .pos(rlayout.fromBottomLeft(120.f, itemsY))
        .parent(m_mainLayer)
        .store(xblIcon);

    Build<CCSprite>::createSpriteName("conn-test-internet-icon.png"_spr)
        .color({255, 255, 255})
        .with([&](auto spr) {
            util::ui::rescaleToMatch(spr, iconSize);
        })
        .pos(rlayout.fromBottomRight(120.f, itemsY))
        .parent(m_mainLayer)
        .store(xblIcon);

    Build<CCSprite>::createSpriteName("conn-test-xbl-icon.png"_spr)
        .with([&](auto spr) {
            util::ui::rescaleToMatch(spr, iconSize);
        })
        .pos(rlayout.fromBottomRight(24.f, itemsY))
        .parent(m_mainLayer)
        .store(xblIcon);

    for (size_t i = 0; i < 15; i++) {
        float baseX = 96.f * static_cast<size_t>(i / 5) + 54.f;
        float x = baseX + 10.f * (i % 5);

        auto spr = Build<CCLabelBMFont>::create(".", "bigFont.fnt")
            .pos(x, itemsY + 8.f)
            .parent(m_mainLayer)
            .collect();

        dotSprites.push_back(spr);
    }

    Build<CCSprite>::createSpriteName("GJ_deleteIcon_001.png")
        .scale(1.1f)
        .visible(false)
        .pos(0.f, itemsY)
        .parent(m_mainLayer)
        .store(crossIcon);

    // status Labels

    float stcellwidth = POPUP_WIDTH * 0.95f;

    auto wrapper = Build<CCNode>::create()
        .contentSize(stcellwidth, POPUP_HEIGHT * 0.6f)
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAxisAlignment(AxisAlignment::End))
        .parent(m_mainLayer)
        .anchorPoint(0.5f, 1.f)
        .pos(rlayout.fromTop(40.f))
        .collect();

    Build<StatusCell>::create("Network:", stcellwidth)
        .parent(wrapper)
        .store(netCell);

    Build<StatusCell>::create("Internet:", stcellwidth)
        .parent(wrapper)
        .store(internetCell);

    Build<StatusCell>::create("Xbox LIVE:", stcellwidth)
        .parent(wrapper)
        .store(srvCell);

    wrapper->updateLayout();
}

void SwagConnectionTestPopup::xbPutCrossAt(size_t idx) {
    if (idx == 0) {
        crossIcon->setVisible(false);
    } else {
        crossIcon->setVisible(true);
        crossIcon->setPositionX(72.5f + 96.f * (idx - 1));
    }
}

void SwagConnectionTestPopup::startTesting() {
    if (xboxStyle) {
        this->xbStartNetwork();
    }

    this->runAction(CCSequence::create(
        CCDelayTime::create(4.0f),
        CallFuncExt::create([this] {
            this->xbStartInternet();
        }),
        nullptr
    ));
}

void SwagConnectionTestPopup::xbStopPulseDots() {
    for (auto dot : this->dotSprites) {
        dot->stopAllActions();
    }
}

void SwagConnectionTestPopup::xbSetDotsForState() {
    size_t greenUntil = (xbCurState - 1) * 5;

    for (size_t i = 0; i < dotSprites.size(); i++) {
        dotSprites[i]->setColor(i < greenUntil ? GREEN_DOT_COLOR : WHITE_DOT_COLOR);
    }
}

void SwagConnectionTestPopup::xbPulseDots() {
    size_t pulseIdxStart = (xbCurState - 1) * 5;
    size_t pulseIdxEnd = xbCurState * 5;

    for (size_t i = 0; i < dotSprites.size(); i++) {
        size_t reli = i % 5;

        if (i >= pulseIdxStart && i < pulseIdxEnd) {
            dotSprites[i]->runAction(
                CCSequence::create(
                    CCDelayTime::create(0.4f * reli),
                    CCRepeat::create(CCSequence::create(
                        CCTintTo::create(0.4f, GREEN_DOT_COLOR.r, GREEN_DOT_COLOR.g, GREEN_DOT_COLOR.b),
                        CCTintTo::create(0.2f, WHITE_DOT_COLOR.r, WHITE_DOT_COLOR.g, WHITE_DOT_COLOR.b),
                        CCDelayTime::create(1.5f),
                        nullptr
                    ), 9999999),
                    nullptr
                )
            );
        }
    }
}

void SwagConnectionTestPopup::xbRedoDots() {
    this->xbStopPulseDots();
    this->xbSetDotsForState();
    this->xbPulseDots();
}

void SwagConnectionTestPopup::xbFail() {
    this->xbStopPulseDots();
    this->xbSetDotsForState();
    this->xbPutCrossAt(this->xbCurState);

    switch (xbCurState) {
        case 1: {
            netCell->setFailed();
            internetCell->setBlocked();
            srvCell->setBlocked();
        } break;

        case 2: {
            internetCell->setFailed();
            srvCell->setBlocked();
        } break;

        case 3: {
            srvCell->setFailed();
        } break;
    }
}

void SwagConnectionTestPopup::xbStartNetwork() {
    this->xbCurState = 1;
    this->xbPutCrossAt(0);
    this->xbRedoDots();
}

void SwagConnectionTestPopup::xbStartInternet() {
    this->xbCurState = 2;
    this->netCell->setSuccessful();
    this->xbRedoDots();

    this->runAction(CCSequence::create(
        CCDelayTime::create(4.0f),
        CallFuncExt::create([this] {
            this->xbFail();
        }),
        nullptr
    ));
}

void SwagConnectionTestPopup::xbStartXbl() {
    this->xbCurState = 3;
    this->internetCell->setSuccessful();
    this->xbRedoDots();
}

void SwagConnectionTestPopup::xbSuccess() {
    this->xbCurState = 4;
    this->xbStopPulseDots();
    this->xbSetDotsForState();
    this->srvCell->setSuccessful();
}

void SwagConnectionTestPopup::animateIn() {
    this->m_noElasticity = true;
    this->show();

    auto winSize = CCDirector::get()->getWinSize();
    auto targetPos = winSize / 2;
    this->setScale(0.f);
    this->setAnchorPoint({0.5f, 0.5f});
    this->ignoreAnchorPointForPosition(false);
    this->setPosition(winSize.width, 0.f);

    this->runAction(CCSequence::create(
        CCSpawn::create(
            CCSequence::create(
                CCMoveBy::create(1.0f, CCPoint{-360.f, 100.f}),
                CCMoveBy::create(1.0f, CCPoint{-120.f, 170.f}),
                CCMoveBy::create(1.0f, CCPoint{200.f, -20.f}),
                CCMoveBy::create(1.0f, CCPoint{300.f, -200.f}),
                CCMoveTo::create(0.8f, targetPos),
                nullptr
            ),

            CCSequence::create(
                CCScaleTo::create(0.5f, 0.2f),
                CCScaleTo::create(2.0f, 0.5f),
                CCScaleTo::create(2.2f, 1.0f),
                nullptr
            ),

            CCSequence::create(
                CCRotateBy::create(4.9f, 720.f * 10.f),
                nullptr
            ),

            nullptr
        ),

        nullptr
    ));
}

void SwagConnectionTestPopup::onClose(CCObject* o) {
    if (reallyClose) {
        Popup::onClose(o);
        return;
    }

    geode::createQuickPopup(
        "Note",
        "Are you sure you want to interrupt the network test?",
        "Cancel",
        "Yes",
        [this](auto, bool yeah) {
            if (!yeah) return;

            this->reallyClose = true;
            this->onClose(this);
        }
    );
}

SwagConnectionTestPopup* SwagConnectionTestPopup::create() {
    auto ret = new SwagConnectionTestPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
