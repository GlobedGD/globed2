#pragma once
#include "basic.hpp"

#include <mutex> // std::call_once and std::once_flag for singletons
#include <memory> // std::unique_ptr for singletons

// singleton classes

#define GLOBED_SINGLETON(cls) \
    private: \
    static std::once_flag __singleton_once_flag; \
    static std::unique_ptr<cls> __singleton_instance; \
    \
    public: \
    static inline cls& get() { \
        std::call_once(__singleton_once_flag, []() { \
            __singleton_instance = std::make_unique<cls>(); \
        }); \
        return *__singleton_instance; \
    } \
    cls(const cls&) = delete; \
    cls& operator=(const cls&) = delete;

#define GLOBED_SINGLETON_DEF(cls) \
    std::unique_ptr<cls> cls::__singleton_instance = std::unique_ptr<cls>(nullptr); \
    std::once_flag cls::__singleton_once_flag;
