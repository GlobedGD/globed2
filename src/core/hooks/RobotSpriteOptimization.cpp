#include <Geode/Geode.hpp>
#include <Geode/modify/SpriteAnimationManager.hpp>
#include <globed/prelude.hpp>

using namespace geode::prelude;

namespace globed {

static CCString* createCCString(std::string_view data) {
    auto str = new CCString();
    str->autorelease();
    str->m_sString = gd::string{data.data(), data.size()};
    return str;
}

struct GLOBED_MODIFY_ATTR SAMHook : Modify<SAMHook, SpriteAnimationManager> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("SpriteAnimationManager::storeAnimation", Priority::Replace);
    }

    void storeAnimation(CCAnimate* action, CCAnimate* frames, gd::string name, int priority, spriteMode type, CCSpriteFrame* first) {
        m_animateDict->setObject(action, name);
        m_frameDict->setObject(frames, name);

        StringBuffer<> firstbuf;
        firstbuf.append("{}_first", name);
        m_frameDict->setObject(first, gd::string{firstbuf.data(), firstbuf.size()});

        StringBuffer<> typebuf;
        typebuf.append("{}", (int)type);
        m_typeDict->setObject(createCCString(typebuf.view()), name);

        StringBuffer<> prioBuf;
        prioBuf.append("{}", priority);
        m_priorityDict->setObject(createCCString(prioBuf.view()), name);
    }
};

}