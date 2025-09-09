#pragma once

#include <qunet/buffers/ByteReader.hpp>
#include <cocos2d.h>

namespace globed {

using Color3 = cocos2d::ccColor3B;

class MultiColor {
public:
    static geode::Result<MultiColor> decode(std::span<const uint8_t> data);
    MultiColor();

    bool operator==(const MultiColor&) const = default;
    bool isMultiple() const;

    /// Throws if `isMultiple()` is false
    const std::vector<Color3>& getColors() const;
    Color3 getColor() const;

    void animateLabel(cocos2d::CCLabelBMFont* label) const;

private:
    enum class Type {
        Static, Tinting, Gradient
    };

    MultiColor(Type type, std::vector<Color3> colors);

    Type m_type;
    std::vector<Color3> m_colors;
};

}