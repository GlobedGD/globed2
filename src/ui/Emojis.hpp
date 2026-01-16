#pragma once

#include <AdvancedLabel.hpp>
#include <unordered_map>

namespace globed {

const Label::EmojiMap *getEmojiMap();
const std::unordered_map<std::string_view, std::u8string_view> &getEmojiTranslationMap();

std::optional<std::string_view> translateEmoji(std::string_view name);
void translateEmojiString(std::string &str);
bool containsEmoji(std::string_view str);

} // namespace globed
