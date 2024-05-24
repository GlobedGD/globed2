#include "misc.hpp"

#include <util/format.hpp>

Result<RichColor> RichColor::parse(std::string_view k) {
    if (k.find('>') == std::string::npos) {
        auto col = util::format::parseColor(k);
        if (!col) return Err(std::move(col.unwrapErr()));

        return Ok(RichColor(col.unwrap()));
    }

    if (k.starts_with('#')) {
        k = k.substr(1, k.size() - 1);
    }

    auto pieces = util::format::split(k, ">");
    std::vector<cocos2d::ccColor3B> colors;

    for (auto piece : pieces) {
        auto col = util::format::parseColor(util::format::trim(piece));
        if (!col) {
            return Err(std::move(col.unwrapErr()));
        }

        colors.emplace_back(col.unwrap());
    }

    return Ok(RichColor(colors));
}
