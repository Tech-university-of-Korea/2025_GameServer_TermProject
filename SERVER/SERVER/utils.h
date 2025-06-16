#pragma once

#include "session.h"
#include "npc.h" 
#include "agro_npc.h"

#include <concepts>

template <class ObjectType> requires std::derived_from<ObjectType, ServerEntity>
struct GetObjectTag;

template <>
struct GetObjectTag<Session> {
    static constexpr ServerObjectTag tag = ServerObjectTag::SESSION;
};

template <>
struct GetObjectTag<PeaceNpc> {
    static constexpr ServerObjectTag tag = ServerObjectTag::PEACE_NPC;
};

template <>
struct GetObjectTag<AgroNpc> {
    static constexpr ServerObjectTag tag = ServerObjectTag::AGRO_NPC;
};

// Util - casting
template <typename T> requires std::derived_from<T, ServerEntity>
inline T* cast_ptr(ServerEntity* obj) 
{
    if (nullptr == obj) {
        return nullptr;
    }

    return GetObjectTag<T>::tag == obj->get_object_tag() ? static_cast<T*>(obj) : nullptr;
}

template <>
inline Npc* cast_ptr(ServerEntity* obj) 
{
    if (nullptr == obj) {
        return nullptr;
    }

    auto tag = obj->get_object_tag();
    bool is_npc = ServerObjectTag::PEACE_NPC == tag or ServerObjectTag::AGRO_NPC == tag;
    return is_npc ? static_cast<Npc*>(obj) : nullptr;
}