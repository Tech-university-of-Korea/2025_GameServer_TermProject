#pragma once

#include "Npc.h"

class AgroNpc : public Npc {
public:
    AgroNpc(int32_t id);
    virtual ~AgroNpc();

public:
    virtual void init_npc_name() override;
    bool initialize_lua_script();

    void process_event_player_move(int32_t target_obj);
    void process_event_npc_move();
    virtual void dispatch_npc_update(IoType type, void* extra_info) override;

private:
    // LUA SCRIPT
    std::mutex _lua_lock{ };
    lua_State* _lua_state{ nullptr };
};
