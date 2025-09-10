#pragma once

#include <qunet/buffers/ByteReader.hpp>
#include <cocos2d.h>

namespace globed {

using Color3 = cocos2d::ccColor3B;

class MultiColor {
public:
    enum class Type {
        Static, Tinting, Gradient
    };

    static geode::Result<MultiColor> decode(std::span<const uint8_t> data);
    MultiColor();
    MultiColor(Type type, std::vector<Color3> colors);

    bool operator==(const MultiColor&) const = default;
    bool isMultiple() const;
    bool isGradient() const;
    bool isTint() const;

    /// Throws if `isMultiple()` is false
    const std::vector<Color3>& getColors() const;
    Color3 getColor() const;

    void animateNode(cocos2d::CCRGBAProtocol* label) const;

private:

    Type m_type;
    std::vector<Color3> m_colors;
};

}