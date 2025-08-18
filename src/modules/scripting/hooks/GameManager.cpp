#include "Common.hpp"
#include <globed/config.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

struct GLOBED_MODIFY_ATTR SCGameManager : Modify<SCGameManager, GameManager> {
    $override
    gd::string stringForCustomObject(int id) {
        auto type = globed::classifyObjectId(id);
        using enum globed::ScriptObjectType;

        switch (type) {
            case None: {
                auto str = GameManager::stringForCustomObject(id);
#ifdef GLOBED_DEBUG
                log::debug("Object string: '{}'", str);
#endif
                return str;
            } break;

            // TODO: improve this, dont hardcode strings
            case FireServer: {
                // -1846870015 is the mask thing
                return "1,3620,2,525,3,75,36,1,476,-16777216,479,1,483,1,482,-1846870015;";
            } break;

            case EmbeddedScript: {
                return "1,914,2,585,3,105,31,R0xPQkVEX1NDUklQVADEGXv6IwAAACi1L_0gIxkBAAAKAHNjcmlwdC5sdWERAAAALS0gWW91ciBjb2RlIGhlcmUA;";
            } break;

            default: break;
        }

        return "";
    }
};