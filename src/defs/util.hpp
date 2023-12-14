#pragma once
#include "basic.hpp"
#include "platform.hpp"
#include <stdexcept>

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
// That makes it not thread safe, and therefore not the best practice.
// But let's be honest, what are the odds of 2 exceptions occurring in two threads at the same time? ;)
//
// Note for future me if this doesn't magically get resolved in 2.2 -
// when throwing a vanilla exception, the abort happens in __cxa_throw, which calls std::terminate() when stack unwinding fails.
// the usual return code for _Unwind_RaiseException in this case is 9, which is _URC_FAILURE, aka unspecified unwinding error.
//
// when throwing a 0 (which is what our workaround does), the crash can still sometimes happen in _Unwind_Resume,
// which is worse because it's an actual segfault and not a call to std::terminate().
// additionally, when throwing anything else (e.g. `throw 1;`) the entire stack may get fucked up and you'll get a useless stack trace.

#ifdef GLOBED_ANDROID

class _GExceptionStore {
public:
    static void store_exc(const char* err) {
        auto len = min(strlen(err), BUF_LEN - 1);
        strncpy(what, err, len);
        what[len] = '\0';
        initialized = true;
    }

    static const char* get() {
        if (!initialized) {
            return "no error";
        }

        return what;
    }

private:
    static constexpr size_t BUF_LEN = 512;
    inline static char what[BUF_LEN];
    inline static bool initialized = false;
};

# define THROW(x) { \
    auto exc = (x); \
    ::_GExceptionStore::store_exc(exc.what()); \
    /* std::rethrow_exception(std::make_exception_ptr(exc)); */ \
    throw 0; } // throwing 0 instead of throwing or rethrowing exc has a higher chance of working

# define CATCH catch(...)
# define CATCH_GET_EXC ::_GExceptionStore::get()

#else // GLOBED_ANDROID

# define THROW(x) throw x;
# define CATCH catch(const std::exception& _caughtException)
# define CATCH_GET_EXC _caughtException.what()

#endif // GLOBED_ANDROID
