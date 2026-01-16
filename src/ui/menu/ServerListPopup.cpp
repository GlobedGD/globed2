#include "ServerListPopup.hpp"
#include "EditServerPopup.hpp"
#include "GlobedMenuLayer.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/ServerManager.hpp>
#include <globed/core/SettingsManager.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ServerListPopup::POPUP_SIZE{340.f, 240.f};
static constexpr CCSize LIST_SIZE{290.f, 180.f};

namespace {
constexpr float CELL_HEIGHT = 32.f;

class ListCell : public CCNode {
public:
    static ListCell *create(const CentralServerData &server, ServerListPopup *popup, size_t idx, bool active)
    {
        auto ret = new ListCell;
        ret->m_popup = popup;
        ret->m_index = idx;
        ret->init(server, active);
        ret->autorelease();
        return ret;
    }

private:
    ServerListPopup *m_popup;
    size_t m_index;

    void init(const CentralServerData &server, bool active)
    {
        this->setContentSize({LIST_SIZE.width, CELL_HEIGHT});

        Build<CCLabelBMFont>::create(server.name.c_str(), "bigFont.fnt")
            .limitLabelWidth(LIST_SIZE.width * 0.6f, 0.6f, 0.1f)
            .anchorPoint(0.f, 0.5f)
            .pos(8.f, CELL_HEIGHT / 2.f)
            .parent(this);

        auto menu = Build<CCMenu>::create()
                        .layout(RowLayout::create()->setAxisAlignment(AxisAlignment::End)->setAutoScale(false))
                        .contentSize(LIST_SIZE.width * 0.35f, CELL_HEIGHT * 0.7f)
                        .anchorPoint(1.f, 0.5f)
                        .pos(LIST_SIZE.width - 8.f, CELL_HEIGHT / 2.f)
                        .parent(this)
                        .collect();

        // main server forced at the first index, cannot be deleted or modified
        if (m_index != 0) {
            Build<CCSprite>::createSpriteName("GJ_deleteBtn_001.png")
                .with([&](auto spr) { cue::rescaleToMatch(spr, CELL_HEIGHT * 0.7f); })
                .intoMenuItem([this](auto) {
                    auto &sm = ServerManager::get();
                    sm.deleteServer(m_index);
                    m_popup->reloadList();
                })
                .parent(menu)
                .id("delete-btn");

            // modify server btn
            Build<CCSprite>::createSpriteName("GJ_editBtn_001.png")
                .with([&](auto spr) { cue::rescaleToMatch(spr, CELL_HEIGHT * 0.7f); })
                .intoMenuItem([this](auto) {
                    auto &sm = ServerManager::get();
                    auto server = sm.getServer(m_index);

                    auto popup = EditServerPopup::create(false, server.name, server.url);
                    popup->setCallback([this, &sm](const std::string &name, const std::string &url) {
                        auto &srv = sm.getServer(m_index);
                        srv.name = name;
                        srv.url = url;
                        sm.commit();
                        m_popup->reloadList();
                    });
                    popup->show();
                })
                .parent(menu)
                .id("modify-btn");
        }

        // switch button

        Build<CCSprite>::createSpriteName(active ? "GJ_selectSongOnBtn_001.png" : "GJ_playBtn2_001.png")
            .with([&](auto spr) { cue::rescaleToMatch(spr, CELL_HEIGHT * 0.7f); })
            .intoMenuItem([this](auto) {
                auto &sm = ServerManager::get();
                sm.switchTo(m_index);
                m_popup->reloadList();
            })
            .parent(menu)
            .id("switch-btn")
            .collect();

        menu->updateLayout();
    }
};

} // namespace

bool ServerListPopup::setup(GlobedMenuLayer *layer)
{
    this->setTitle("Edit Servers");

    m_menuLayer = layer;

    m_list = Build(cue::ListNode::create(LIST_SIZE)).pos(this->fromCenter(0.f, -10.f)).parent(m_mainLayer);
    m_list->setCellHeight(CELL_HEIGHT);

    this->reloadList();

    return true;
}

void ServerListPopup::reloadList()
{
    m_list->clear();

    auto &sm = ServerManager::get();
    size_t activeIdx = sm.getActiveIndex();
    auto &servers = sm.getAllServers();

    size_t i = 0;
    for (auto &server : servers) {
        m_list->addCell(ListCell::create(server, this, i, activeIdx == i));
        i++;
    }

    auto *btn =
        Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
            .with([&](auto btn) { cue::rescaleToMatch(btn, CCSize{CELL_HEIGHT * 0.85f, CELL_HEIGHT * 0.85f}); })
            .intoMenuItem([this] {
                if (!globed::swapFlag("core.flags.seen-add-server-warn")) {
                    globed::alert("Notice", "If you want to make your own <cy>custom server</c>, you need hosting. "
                                            "Hosting instructions can be found on the <cj>Globed GitHub page</c>.");

                    return;
                }

                auto &sm = ServerManager::get();

                auto popup = EditServerPopup::create(true, "", "");
                popup->setCallback([this, &sm](const std::string &name, const std::string &url) {
                    sm.addServer({.name = name, .url = url});
                    this->reloadList();
                });
                popup->show();
            })
            .scaleMult(1.1f)
            .collect();

    auto menu = Build<CCMenu>::create()
                    .contentSize(LIST_SIZE.width, CELL_HEIGHT)
                    .ignoreAnchorPointForPos(false)
                    .intoNewChild(btn)
                    .pos(LIST_SIZE.width / 2.f, CELL_HEIGHT / 2.f)
                    .intoParent()
                    .collect();

    m_list->addCell(menu);

    // additionally force reload the main layer
    m_menuLayer->onServerModified();
}

} // namespace globed