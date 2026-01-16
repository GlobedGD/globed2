#pragma once

namespace globed {

class BoolExt {
public:
    constexpr inline BoolExt(bool val = false) : m_val(val) {}
    constexpr inline BoolExt(const BoolExt &other) = default;
    constexpr inline BoolExt(BoolExt &&other) noexcept = default;
    constexpr inline BoolExt &operator=(const BoolExt &other) = default;
    constexpr inline BoolExt &operator=(BoolExt &&other) noexcept = default;

    constexpr inline operator bool() const
    {
        return m_val;
    }

    constexpr inline BoolExt &operator=(bool val)
    {
        m_val = val;
        return *this;
    }

    constexpr inline bool take()
    {
        bool old = m_val;
        m_val = false;
        return old;
    }

private:
    bool m_val;
};

} // namespace globed