#pragma once

#include <string>
#include <string.h>

namespace globed {

// Like a `string_view`, but must be null terminated. Does not own the memory it points to, does not copy anything.
// Can be constructed from a `std::string` or a `const char*`. It is not safe to use `string_view.data()` to construct this.
// Can never hold a null pointer, converts it into an empty string pointer.
class CStr {
public:
    inline CStr() : m_ptr("") {}
    inline CStr(const char* str) : m_ptr(str ? str : "") {}
    inline CStr(const std::string& str) : m_ptr(str.c_str()) {}

    inline CStr(const CStr& other) = default;
    inline CStr& operator=(const CStr& other) = default;

    inline operator const char*() const {
        return m_ptr;
    }

    inline operator std::string_view() const {
        return m_ptr;
    }

    inline operator bool() const {
        return !this->empty();
    }

    inline const char* get() const {
        return m_ptr;
    }

    // Returns the pointer if the string is not empty, otherwise returns nullptr.
    inline const char* getOrNull() const {
        return m_ptr[0] ? m_ptr : nullptr;
    }

    inline size_t size() const {
        return strlen(m_ptr);
    }

    inline bool empty() const {
        return m_ptr[0] == '\0';
    }

private:
    const char* m_ptr;
};

}
