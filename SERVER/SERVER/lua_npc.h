#pragma once

#include "Npc.h"

class LuaNpc : public Npc {
public:
    LuaNpc(int32_t id);
    virtual ~LuaNpc();

public:
    bool initialize_lua_script();

    void process_event_player_move(int32_t target_obj);
    void process_event_npc_move();
    virtual void process_game_event(GameEvent* event) override;
    virtual void dispatch_npc_update(COMP_TYPE type) override;

private:
    // LUA SCRIPT
    std::mutex _lua_lock{ };
    lua_State* _lua_state{ nullptr };
};
