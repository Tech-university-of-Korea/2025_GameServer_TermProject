#pragma once

#include "server_entity.h"

class Npc : public ServerEntity {
public:
    Npc(ServerObjectTag tag, int32_t id);
    virtual ~Npc();

public:
    virtual void init_npc_name() abstract;

    void do_npc_move(int32_t move_dx, int32_t move_dy);
    void do_npc_random_move();

    bool check_player_in_view_range();
    void chase_target();
    void attack();
    virtual void process_event_npc_move();
    virtual void process_game_event(GameEvent* event) override;

    virtual void dispatch_npc_update(IoType type, void* extra_info);

protected:
    int32_t _target_player{ SYSTEM_ID };
};