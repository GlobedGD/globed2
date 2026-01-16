#pragma once

#include <modules/scripting/data/EmbeddedScript.hpp>
#include <modules/scripting/objects/Ids.hpp>

namespace globed {

ScriptObjectType classifyObjectId(int objectIdRaw);
const char *textureForScriptObject(ScriptObjectType type);

bool onAddObject(GameObject *p0, bool editor, std::optional<EmbeddedScript> &outScripts);
bool onEditObject(GameObject *p0);
// void onCreateObject(ItemTriggerGameObject* obj, ScriptObjectType type);
void postCreateObject(GameObject *obj, ScriptObjectType type);
void onUpdateObjectLabel(GameObject *obj);

static constexpr auto ALL_SCRIPT_OBJECT_TYPES = std::to_array<ScriptObjectType>({
    ScriptObjectType::FireServer,
    ScriptObjectType::EmbeddedScript,
});

} // namespace globed
