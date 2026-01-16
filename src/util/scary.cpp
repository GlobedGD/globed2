#include <globed/util/scary.hpp>

using namespace geode::prelude;

namespace globed {

void setObjectTypeName(CCNode *obj, std::string_view name)
{
#ifdef GEODE_IS_WINDOWS
    auto cstr = new CCString();
    cstr->autorelease();
    cstr->m_sString = name;
    obj->setUserObject("rttiName"_spr, cstr);

    struct RTTILocator {
        uint32_t signature;
        uint32_t offset;
        uint32_t cdOffset;
        uint32_t typeDescriptor;
        uint32_t classHierarchyDescriptor;
        uint32_t self;
    };

    struct RTTIDescriptor {
        void *pVFTable;
        void *spare;
        char name[];
    };

    void **vtable = *reinterpret_cast<void ***>(obj);
    auto rttiLocator = *reinterpret_cast<RTTILocator **>(vtable - 1);
    auto base = (uintptr_t)rttiLocator - rttiLocator->self;
    auto desc = (RTTIDescriptor *)(base + (uintptr_t)rttiLocator->typeDescriptor);

    void **spare = &desc->spare;
    DWORD old;
    if (!VirtualProtect(spare, sizeof(void *), PAGE_READWRITE, &old)) {
        log::error("Failed to change memory protection for spare at {:x} ({})", (uintptr_t)spare, GetLastError());
        return;
    }

    *spare = (void *)cstr->m_sString.c_str();

    VirtualProtect(spare, sizeof(void *), old, &old);
#endif
}

} // namespace globed