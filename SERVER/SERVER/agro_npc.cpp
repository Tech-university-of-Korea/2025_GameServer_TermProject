#include "pch.h"
#include "agro_npc.h"
#include "lua_api.h"

AgroNpc::AgroNpc(int32_t id) : Npc{ ServerObjectTag::AGRO_NPC, id } {}

AgroNpc::~AgroNpc() { }

void AgroNpc::process_event_player_move(int32_t target_obj) {
	std::lock_guard lua_guard{ _lua_lock };
	lua_getglobal(_lua_state, "event_player_move");
	lua_pushnumber(_lua_state, target_obj);

	if (LUA_OK != lua_pcall(_lua_state, 1, 1, 0)) {
		std::cout << lua_tostring(_lua_state, -1) << "\n";
		lua_pop(_lua_state, 1);
		return;
	}

	_target_player = lua_tointeger(_lua_state, -1);
	lua_pop(_lua_state, 1);
	return;
}

void AgroNpc::process_event_npc_move() {
	if (_target_player == SYSTEM_ID or _target_player >= MAX_USER) {
		_target_player = SYSTEM_ID;
		do_npc_random_move();
	}
	else {
		chase_target();
	}

	g_server.add_timer_event(_id, NPC_MOVE_TIME, OP_NPC_MOVE);
}

void AgroNpc::dispatch_npc_update(IoType type, void* extra_info) { 
	switch (type) {
	case OP_NPC_MOVE:
	{
		if (false == is_active()) {
			return;
		}

		bool keep_alive = check_player_in_view_range();
		if (true == keep_alive) {
			process_event_npc_move();
		}
		else {
			update_active_state(false);
		}
	}
	break;

	case OP_NPC_RESPAWN:
	{
		std::cout << std::format("Respawn npc: {}\n", _id);
		if (true == try_respawn(100)) {
			auto [x, y] = get_position();
			g_sector.insert(_id, x, y);
		}
	}
	break;

	case OP_NPC_CHECK_AGRO:
	{
		process_event_player_move(reinterpret_cast<int64_t>(extra_info));
	}
	break;

	default:
		break;
	}
}

void AgroNpc::init_npc_name() {
	std::memset(_name, 0, MAX_ID_LENGTH);
	std::memcpy(_name, "Agro NPC", std::strlen("Agro NPC"));
	_state = ST_INGAME;

	_level = rand() % 20 + 1;
	_max_hp = _level * 100;
	_hp = _max_hp;
}

bool AgroNpc::initialize_lua_script() {
	_lua_state = luaL_newstate();
	luaL_openlibs(_lua_state);
	lua_register(_lua_state, "API_get_x", API_get_x);
	lua_register(_lua_state, "API_get_y", API_get_y);

	if (LUA_OK != luaL_loadfile(_lua_state, "npc.lua") or LUA_OK != lua_pcall(_lua_state, 0, 0, 0)) {
		std::cout << lua_tostring(_lua_state, -1) << "\n";
		lua_pop(_lua_state, 1);
		return false;
	}

	lua_getglobal(_lua_state, "set_uid");
	lua_pushnumber(_lua_state, _id);
	lua_pcall(_lua_state, 1, 0, 0);
	//lua_pop(L, 1);// eliminate set_uid from stack after call

	return true;
}