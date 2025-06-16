#include "pch.h"
#include "lua_api.h"

int32_t API_get_x(lua_State* L) 
{
	int32_t user_id = (int32_t)lua_tointeger(L, -1);
	lua_pop(L, 2);

	auto client = g_server.get_server_object(user_id);
	if (nullptr == client) {
		lua_pushnumber(L, -1);
		return 1;
	}

	int32_t x = client->get_x();
	lua_pushnumber(L, x);
	return 1;
}

int32_t API_get_y(lua_State* L)
{
	int32_t user_id = (int32_t)lua_tointeger(L, -1);
	lua_pop(L, 2);

	auto client = g_server.get_server_object(user_id);
	if (nullptr == client) {
		lua_pushnumber(L, -1);
		return 1;
	}

	int y = client->get_y();
	lua_pushnumber(L, y);
	return 1;
}