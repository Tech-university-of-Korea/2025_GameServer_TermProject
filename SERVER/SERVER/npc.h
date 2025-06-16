#pragma once

#include "server_entity.h"

class Npc : public ServerObject {
public:
    Npc(int32_t id);
    Npc(ServerObjectTag tag, int32_t id);
    virtual ~Npc();

public:
    void init_npc_name(std::string_view name);

    void do_npc_move(int32_t move_dx, int32_t move_dy);
    void do_npc_random_move();

    bool check_player_in_view_range();

    virtual void process_event_npc_move();
    virtual void process_game_event(GameEvent* event) override;

    virtual void dispatch_npc_update(IoType type);
};