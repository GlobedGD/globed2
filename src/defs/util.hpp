#pragma once
#include "basic.hpp"
#include "platform.hpp"
#include <stdexcept>

#define GLOBED_MBO(src, type, offset) *(type*)((char*)src + offset)

// singleton classes

// there was no reason to do this other than for me to learn crtp
template <typename Derived>
class SingletonBase {
public:
    // no copy
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    // no move
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

    static Derived& get() {
        static Derived instance;
        return instance;
    }

protected:
    SingletonBase() = default;
    virtual ~SingletonBase() = default;
};

#define GLOBED_SINGLETON(cls) public SingletonBase<cls>

// exception handler (i cannot handle this language anymore dude)
// explanation for people with sanity:
// tl;dr - https://discord.com/channels/911701438269386882/979402752121765898/1183817498668318891
//
// on android, for some reason, exception handlers crash in various weird circumstances.
// adding -fexceptions doesn't work, and neither does anything else i've tried.
// thanks to this workaround, whenever you throw an exception, it sets a global variable to the exception message string,
// so that you can do `catch(...)` (shorthand - CATCH) but still get the exception message via shorthand `CATCH_GET_EXC`.
//
// Note for future me if this doesn't magically get resolved in 2.2 -
// when throwing a vanilla exception, the abort happens in __cxa_throw, which calls std::terminate() when stack unwinding fails.
// the usual return code for _Unwind_RaiseException in this case is 9, which is _URC_FAILURE, aka unspecified unwinding error.
//
// when throwing a 0 (which is what our workaround does), the crash can still sometimes happen in _Unwind_Resume,
// which is worse because it's an actual segfault and not a call to std::terminate().
// additionally, when throwing anything else (e.g. `throw 1;`) the entire stack may get fucked up and you'll get a useless stack trace.

#if 0

// TODO none of this works on android !!!!!!!

# include <stack>
# include <cstring>
# include <csetjmp>

static inline thread_local std::jmp_buf _globedErrJumpBuf;

struct _JMPBufWrapper {
    std::jmp_buf buf;
};

class _JMPBufStack {
public:
    static bool push(std::jmp_buf buf) {
        _stack.push(_JMPBufWrapper {.buf = { buf[0] }});
        return true;
    }

    static _JMPBufWrapper pop() {
        auto buf = _stack.top();
        _stack.pop();
        return buf;
    }

private:
    inline static std::stack<_JMPBufWrapper> _stack;
};

struct _GExceptionWrapper {
public:
    static void store(const char* err) {
        auto len = std::min(std::strlen(err), BUF_LEN - 1);
        strncpy(whatbuf, err, len);
        whatbuf[len] = '\0';
        initialized = true;
    }

    static const char* what() {
        return whatbuf;
    }

    operator bool() {
        return true;
    }

private:
    static constexpr size_t BUF_LEN = 512;
    inline static char whatbuf[BUF_LEN];
    inline static bool initialized = false;
};

class _GExceptionStore {
public:
    static void store(const char* err) {
        exception.store(err);
    }

    static _GExceptionWrapper get() {
        return exception;
    }

private:
    inline thread_local static _GExceptionWrapper exception;
};

# define try if (setjmp(_globedErrJumpBuf) == 0 && _JMPBufStack::push(_globedErrJumpBuf)) {
# define catch(evar) ; _JMPBufStack::pop(); } else if (auto evar = _GExceptionStore::get())
# define throw(exc) { auto _gtexc = (exc); _GExceptionStore::store(_gtexc.what()); std::longjmp(_JMPBufStack::pop().buf, 1); }

#else // GLOBED_ANDROID

# define catch(evar) catch(const std::exception& evar)
# define throw(x) throw x

#endif // GLOBED_ANDROID
