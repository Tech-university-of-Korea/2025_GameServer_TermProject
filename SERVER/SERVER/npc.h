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

    virtual void process_event_npc_move();
};