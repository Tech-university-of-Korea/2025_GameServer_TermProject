#include "pch.h"
#include "peace_npc.h"

PeaceNpc::PeaceNpc(int32_t id) 
    : Npc{ ServerObjectTag::PEACE_NPC, id } { }

PeaceNpc::~PeaceNpc() { }

void PeaceNpc::init_npc_name() {
	std::memset(_name, 0, MAX_ID_LENGTH);
	std::memcpy(_name, "Peace NPC", std::strlen("Peace NPC"));
	_state = ST_INGAME;

	_level = rand() % 40 + 1;
	_max_hp = _level * 100;
	_hp = _max_hp;
}
