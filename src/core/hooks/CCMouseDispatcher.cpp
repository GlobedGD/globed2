#include <Geode/Geode.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <asp/collections/SmallVec.hpp>
#include <asp/time/Instant.hpp>

#ifndef GEODE_IS_IOS

#include <ui/settings/SettingsLayer.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace {

void getParentChain(asp::SmallVec<CCNode*, 64>& out, CCNode* node) {
    while (node) {
        out.push_back(node);
        node = node->getParent();
    }
}

void sortCandidates(auto& candidates) {
    std::sort(candidates.begin(), candidates.end(),
        [](ScrollLayer* a, ScrollLayer* b) {
            asp::SmallVec<CCNode*, 64> aParents, bParents;
            getParentChain(aParents, a);
            getParentChain(bParents, b);

            // find the deepest common parent, and na + nb will be its descendants that lead to a and b
            size_t i = 0;
            CCNode* na = nullptr;
            CCNode* nb = nullptr;
            while (true) {
                if (i >= aParents.size() || i >= bParents.size()) {
                    break;
                }

                na = aParents[aParents.size() - 1 - i];
                nb = bParents[bParents.size() - 1 - i];

                if (na != nb) {
                    break;
                }
                i++;
            }

            if (!na || !nb) {
                return false; // should never happen
            }

            int za = na->getZOrder();
            int zb = nb->getZOrder();

            if (za != zb) {
                return za > zb;
            }

            return na->getOrderOfArrival() > nb->getOrderOfArrival();
        }
    );
}

struct GLOBED_NOVTABLE CCMouseDispatcherH : Modify<CCMouseDispatcherH, CCMouseDispatcher> {
    static void onModify(auto& self) {
        // to fix devtools
        (void) self.setHookPriority("cocos2d::CCMouseDispatcher::dispatchScrollMSG", 100).unwrap();
    }

	bool dispatchScrollMSG(float x, float y) {
        auto start = Instant::now();

        // only touch globed layers!
        auto scene = CCScene::get();
        auto children = scene->getChildrenExt();
        bool applyChanges = false;

        if (children.size() > 0) {
            auto layer = children[0];
            applyChanges =
                typeinfo_cast<globed::SettingsLayer*>(layer)
                || typeinfo_cast<globed::GlobedMenuLayer*>(layer);
        }

        if (!applyChanges) {
            return CCMouseDispatcher::dispatchScrollMSG(x, y);
        }

        asp::SmallVec<ScrollLayer*, 32> candidates;

        for (CCMouseHandler* handler : CCArrayExt<CCMouseHandler*>(m_pMouseHandlers)) {
            auto* scroll = typeinfo_cast<ScrollLayer*>(handler->getDelegate());
            if (!scroll) continue;
            if (!cocos::nodeIsVisible(scroll)) continue;

            // // only modify lists made by cue
            // auto listNode = typeinfo_cast<cue::ListNode*>(scroll->getParent());
            // if (!listNode) continue;
            candidates.push_back(scroll);
        }

        sortCandidates(candidates);

        if (candidates.empty()) {
            return CCMouseDispatcher::dispatchScrollMSG(x, y);
        } else {
            candidates[0]->scrollWheel(x, y);
            return true;
        }
    }
};
}

#endif
