#pragma once

#include <Geode/Result.hpp>
#include <cocos2d.h>
#include <span>
#include <vector>

namespace globed {

using Color3 = cocos2d::ccColor3B;

class MultiColor {
public:
    enum class Type {
        Static, Tinting, Gradient
    };

    static geode::Result<MultiColor> decode(std::span<const uint8_t> data);
    static MultiColor fromColor(const Color3& color);

    MultiColor();
    MultiColor(Type type, std::vector<Color3> colors);

    MultiColor(const MultiColor&) = default;
    MultiColor& operator=(const MultiColor&) = default;
    MultiColor(MultiColor&&) noexcept = default;
    MultiColor& operator=(MultiColor&&) noexcept = default;

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