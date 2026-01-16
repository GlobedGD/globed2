
#include <globed/config.hpp>
#include <modules/scripting-ui/ScriptingUIModule.hpp>
#include <modules/scripting/hooks/Common.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

struct ObjectStringBuilder {
    ObjectStringBuilder &prop(int key, auto &&value)
    {
        if (!_inner.empty())
            _inner.push_back(',');
        _inner += fmt::format("{},{}", key, value);
        return *this;
    }

#define osgetter(g, id)                                                                                                \
    ObjectStringBuilder &g(int value)                                                                                  \
    {                                                                                                                  \
        return prop(id, value);                                                                                        \
    }
    osgetter(objectId, 1);
    osgetter(x, 2);
    osgetter(y, 3);
#undef osgetter

    gd::string build()
    {
        _inner.push_back(';');
#ifdef GEODE_IS_ANDROID
        return _inner;
#else
        return std::move(_inner);
#endif
    }

    std::string _inner;
};

struct GLOBED_MODIFY_ATTR SCGameManager : Modify<SCGameManager, GameManager> {
    static void onModify(auto &self)
    {
        GLOBED_CLAIM_HOOKS(globed::ScriptingUIModule::get(), self, "GameManager::stringForCustomObject", );
    }

    $override gd::string stringForCustomObject(int id)
    {
        auto type = globed::classifyObjectId(id);
        using enum globed::ScriptObjectType;

        switch (type) {
        case None: {
            return GameManager::stringForCustomObject(id);
        } break;

        case FireServer: {
            // -1846870015 is the mask thing
            return ObjectStringBuilder{}
                .objectId(3620)
                .x(525)
                .y(75)
                .prop(36, 1)
                .prop(476, -16777216)
                .prop(479, 1)
                .prop(483, 1)
                .prop(482, -1846870015)
                .build();
        } break;

        case EmbeddedScript: {
            return ObjectStringBuilder{}
                .objectId(914)
                .x(585)
                .y(105)
                .prop(31, "R0xPQkVEX1NDUklQVADEGXv6IwAAACi1L_0gIxkBAAAKAHNjcmlwdC5sdWERAAAALS0gWW91ciBjb2RlIGhlcmUA")
                .build();
        } break;

        default:
            break;
        }

        return "";
    }
};