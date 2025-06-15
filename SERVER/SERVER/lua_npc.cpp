#include "pch.h"
#include "lua_npc.h"
#include "lua_api.h"

LuaNpc::LuaNpc(int32_t id) : Npc{ ServerObjectTag::LUA_NPC, id } {}

LuaNpc::~LuaNpc() { }

void LuaNpc::process_event_player_move(int32_t target_obj) {
	std::lock_guard lua_guard{ _lua_lock };
	lua_getglobal(_lua_state, "event_player_move");
	lua_pushnumber(_lua_state, target_obj);

	if (LUA_OK != lua_pcall(_lua_state, 1, 0, 0)) {
		std::cout << lua_tostring(_lua_state, -1) << "\n";
		lua_pop(_lua_state, 1);
		return;
	}
}

void LuaNpc::process_event_npc_move() {
	bool state{ false };
	int32_t dx{ };
	int32_t dy{ };
	{
		std::lock_guard lua_guard{ _lua_lock };
		lua_getglobal(_lua_state, "event_do_npc_random_move");
		if (LUA_OK != lua_pcall(_lua_state, 0, 0, 0)) {
			std::cout << lua_tostring(_lua_state, -1) << "\n";
			lua_pop(_lua_state, 1);
			return;
		}

		lua_getglobal(_lua_state, "state_collision_with_player");
		lua_getglobal(_lua_state, "move_dx");
		lua_getglobal(_lua_state, "move_dy");
		state = (bool)lua_toboolean(_lua_state, -3);
		dx = (int32_t)lua_tointeger(_lua_state, -2);
		dy = (int32_t)lua_tointeger(_lua_state, -1);

		lua_pop(_lua_state, 3);
	}

	if (false == state) {
		do_npc_random_move();
	}
	else {
		do_npc_move(dx, dy);
	}

	g_server.add_timer_event(_id, 1s, OP_NPC_MOVE);
}

void LuaNpc::process_game_event(GameEvent* event) {
}

void LuaNpc::dispatch_npc_update(COMP_TYPE type) {
	//process_event_player_move();
}

bool LuaNpc::initialize_lua_script() {
	_lua_state = luaL_newstate();
	luaL_openlibs(_lua_state);
	lua_register(_lua_state, "API_send_message", API_send_message);
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