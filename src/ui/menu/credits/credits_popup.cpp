#include "credits_popup.hpp"
#include <util/ui.hpp>

#include "credits_player.hpp"
#include "credits_cell.hpp"

using namespace geode::prelude;

bool GlobedCreditsPopup::setup() {
    using Icons = GlobedSimplePlayer::Icons;

    this->setTitle("Credits");

    auto rlayout = util::ui::getPopupLayout(m_size);

    auto* listlayer = Build<GJCommentListLayer>::create(nullptr, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false)
        .pos((m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2, 60.f)
        .parent(m_mainLayer)
        .collect();

    auto* scrollLayer = Build(ScrollLayer::create({LIST_WIDTH, LIST_HEIGHT}))
        .parent(listlayer)
        .collect();

    scrollLayer->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoScale(false)
    );

#define ADD_PLAYER(array, name, accountid, userid, cube, col1, col2, col3) \
    array->addObject( \
        GlobedCreditsPlayer::create(name, accountid, userid, Icons { .type = IconType::Cube, .id = cube, .color1 = col1, .color2 = col2, .color3 = col3}) \
    )

    /* Owners */

    CCArray* owners = CCArray::create();

    ADD_PLAYER(owners, "dankmeme01", 9735891, 88588343, 1, 41, 16, -1);
    ADD_PLAYER(owners, "availax", 1621348, 8463710, 373, 6, 52, 52);

    auto* cellOwners = Build<GlobedCreditsCell>::create("Owner", false, owners)
        .id("cell-owner"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    /* Staff */

    CCArray* staff = CCArray::create();

    ADD_PLAYER(staff, "Croozington", 18950870, 172707109, 120, 13, 10, 98);
    ADD_PLAYER(staff, "Xanii", 1176802, 7500092, 4, 19, 12, 12);

    ADD_PLAYER(staff, "ddest1nyy", 1249399, 8533459, 461, 96, 62, 70);
    ADD_PLAYER(staff, "Dasshu", 1975253, 7855157, 373, 98, 12, 12);
    ADD_PLAYER(staff, "Lxwnar", 25517837, 153455117, 132, 52, 12, 52);
    ADD_PLAYER(staff, "BoCKTheBook", 6209270, 20531100, 272, 6, 106, -1);
    ADD_PLAYER(staff, "ItzKiba", 4569963, 15447500, 110, 6, 10, 42);
    ADD_PLAYER(staff, "NotTerma", 4706010, 16077956, 379, 98, 36, 43);
    ADD_PLAYER(staff, "ciyaqil", 13640460, 127054504, 84, 12, 40, 40);
    ADD_PLAYER(staff, "TheKiroshi", 6442871, 31833361, 1, 8, 11, 63);
    ADD_PLAYER(staff, "Jiflol", 8262191, 61991331, 35, 7, 3, -1);
    ADD_PLAYER(staff, "LimeGradient", 7214334, 42454266, 37, 2, 12, 2);
    ADD_PLAYER(staff, "ManagerMagolor", 25450870, 195501521, 101, 43, 7, -1);

    ADD_PLAYER(staff, "Capeling", 18226543, 160504868, 141, 76, 40, 40);
    ADD_PLAYER(staff, "Awesomeoverkill", 7479054, 47290058, 77, 1, 5, -1);
    ADD_PLAYER(staff, "skrunkly", 14462068, 146500573, 37, 17, 19, -1);
    ADD_PLAYER(staff, "NikoSando", 737532, 4354472, 433, 16, 6, 12);

    auto* cellStaff = Build<GlobedCreditsCell>::create("Staff", true, staff)
        .id("cell-staff"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    /* Contributor */

    CCArray* contributors = CCArray::create();

    ADD_PLAYER(contributors, "Awesomeoverkill", 7479054, 47290058, 77, 1, 5, -1);
    ADD_PLAYER(contributors, "Capeling", 18226543, 160504868, 141, 76, 40, 40);
    ADD_PLAYER(contributors, "TheSillyDoggo", 16778880, 162245660, 5, 41, 40, 12);
    ADD_PLAYER(contributors, "angeld233", 28024715, 236381167, 98, 4, 12, 12);

    auto* cellContrib = Build<GlobedCreditsCell>::create("Contributor", false, contributors)
        .id("cell-contributor"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    /* Special */

    CCArray* special = CCArray::create();

    ADD_PLAYER(special, "MathieuAR", 3759035, 13781237, 220, 5, 9, 106);
    ADD_PLAYER(special, "HJfod", 104257, 1817908, 132, 8, 3, 40);
    ADD_PLAYER(special, "Cvolton", 761691, 6330800, 101, 8, 12, -1);
    ADD_PLAYER(special, "alk1m123", 11535118, 116938760, 102, 12, 12, -1);
    ADD_PLAYER(special, "mat4", 5568872, 19127913, 81, 0, 20, 2);

    auto* cellSpecial = Build<GlobedCreditsCell>::create("Special thanks", true, special)
        .id("cell-special"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    scrollLayer->m_contentLayer->setContentHeight(
        cellOwners->getScaledContentSize().height
        + cellStaff->getScaledContentSize().height
        + cellContrib->getScaledContentSize().height
        + cellSpecial->getScaledContentSize().height
    );
    scrollLayer->m_contentLayer->updateLayout();

    // make the list start at the top instead of the bottom
    scrollLayer->m_contentLayer->setPositionY(
        scrollLayer->getScaledContentSize().height - scrollLayer->m_contentLayer->getScaledContentSize().height
    );

    return true;
}

GlobedCreditsPopup* GlobedCreditsPopup::create() {
    auto ret = new GlobedCreditsPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}