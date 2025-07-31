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

    operator const char*() const {
        return m_ptr;
    }

    const char* get() const {
        return m_ptr;
    }

    size_t size() const {
        return strlen(m_ptr);
    }

    bool empty() const {
        return m_ptr[0] == '\0';
    }

private:
    const char* m_ptr;
};

}
