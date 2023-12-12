#pragma once
#include "basic.hpp"
#include "platform.hpp"
#include <stdexcept>

// singleton classes

template <typename Derived>
class SingletonBase {
public:
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;

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
    static void throw_exc(const char* err) {
        if (what != nullptr) {
            delete[] what;
        }

        auto len = strlen(err);
        what = new char[len + 1];
        strcpy(what, err);
        what[len] = '\0';
    }

    static const char* get() {
        return what ? what : "no error";
    }

private:
    inline static char* what = nullptr;
};

# define THROW(x) { \
    auto exc = (x); \
    ::_GExceptionStore::throw_exc(exc.what()); \
    /* std::rethrow_exception(std::make_exception_ptr(exc)); */ \
    throw 0; } // throwing 0 instead of exc has a higher chance of working

# define CATCH catch(...)
# define CATCH_GET_EXC ::_GExceptionStore::get()

#else // GLOBED_ANDROID

# define THROW(x) throw x;
# define CATCH catch(const std::exception& _caughtException)
# define CATCH_GET_EXC _caughtException.what()

#endif // GLOBED_ANDROID
