#pragma once

#include "npc.h"

class PeaceNpc : public Npc { 
public:
    PeaceNpc(int32_t id);
    virtual ~PeaceNpc();

public:
    virtual void init_npc_name() override;
};

