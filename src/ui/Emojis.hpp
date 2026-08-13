#pragma once

#include <Geode/ui/Label.hpp>
#include <unordered_map>

namespace globed {

geode::EmojiRegistry& getEmojiMap();
const std::unordered_map<std::string_view, std::u8string_view>& getEmojiTranslationMap();

std::optional<std::string_view> translateEmoji(std::string_view name);
void translateEmojiString(std::string& str);
bool containsEmoji(std::string_view str);

}
