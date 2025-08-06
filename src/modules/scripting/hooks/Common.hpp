#pragma once

#include <modules/scripting/objects/Ids.hpp>

namespace globed {

ScriptObjectType classifyObjectId(int objectIdRaw);
const char* textureForScriptObject(ScriptObjectType type);

bool onAddObject(GameObject* p0, bool editor);
bool onEditObject(GameObject* p0);
void onCreateObject(ItemTriggerGameObject* obj, ScriptObjectType type);


static constexpr auto ALL_SCRIPT_OBJECT_TYPES = std::to_array<ScriptObjectType>({
    ScriptObjectType::FireServer,
    // ScriptObjectType::ListenEvent,
});

}
