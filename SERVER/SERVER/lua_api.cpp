#include "pch.h"
#include "lua_api.h"

int32_t API_get_x(lua_State* L) 
{
	int32_t user_id = (int32_t)lua_tointeger(L, -1);
	lua_pop(L, 2);

	auto client = g_server.get_session(user_id);
	if (nullptr == client) {
		return 0;
	}

	int32_t x = client->get_x();
	lua_pushnumber(L, x);
	return 1;
}

int32_t API_get_y(lua_State* L)
{
	int32_t user_id = (int32_t)lua_tointeger(L, -1);
	lua_pop(L, 2);

	auto client = g_server.get_session(user_id);
	if (nullptr == client) {
		return 0;
	}

	int y = client->get_y();
	lua_pushnumber(L, y);
	return 1;
}

int32_t API_send_message(lua_State* L)
{
	int32_t my_id = (int32_t)lua_tointeger(L, -3);
	int32_t user_id = (int32_t)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	auto client = g_server.get_session(user_id);
	if (nullptr == client) {
		return 0;
	}

	client->send_chat_packet(my_id, mess);
	return 0;
}
