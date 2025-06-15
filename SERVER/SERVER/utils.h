#pragma once

#include "session.h"
#include "npc.h" 
#include "server_object_tags.h"
#include "lua_npc.h"

#include <concepts>

template <class ObjectType> requires std::derived_from<ObjectType, ServerObject>
struct GetObjectTag;

template <>
struct GetObjectTag<Session> {
    static constexpr ServerObjectTag tag = ServerObjectTag::SESSION;
};

template <>
struct GetObjectTag<Npc> {
    static constexpr ServerObjectTag tag = ServerObjectTag::NPC;
};

template <>
struct GetObjectTag<LuaNpc> {
    static constexpr ServerObjectTag tag = ServerObjectTag::LUA_NPC;
};

// Util - casting
template <typename T> requires std::derived_from<T, ServerObject>
inline T* cast_ptr(ServerObject* obj) {
    if (nullptr == obj) {
        return nullptr;
    }

    return GetObjectTag<T>::tag == obj->get_object_tag() ? static_cast<T*>(obj) : nullptr;
}