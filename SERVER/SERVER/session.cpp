#include "pch.h"
#include "session.h"
#include "lua_api.h"

Session::Session(SOCKET socket, int32_t id) 
	: _socket{ socket }, _id{ id }, _name{ } { }

Session::~Session() { }

bool Session::is_player() {
	return _id < MAX_USER;
}

bool Session::is_npc() {
	return _id >= MAX_USER;
}	

bool Session::is_active() {
	return _is_active;
}

int16_t Session::get_x() {
	return _x;
}

int16_t Session::get_y() {
	return _y;
}

std::pair<int16_t, int16_t> Session::get_position() {
	return std::make_pair(_x, _y);
}

std::string_view Session::get_name() {
	return _name;
}

int32_t Session::get_hp() {
	return _hp;
}

bool Session::try_respawn(int32_t max_hp) {
	auto old_hp = _hp.load();
	return _hp.compare_exchange_strong(old_hp, max_hp);
}

void Session::update_hp(int32_t diff) {
	_hp.fetch_add(diff);
}

S_STATE Session::get_state() {
	return _state;
}

void Session::update_position(int16_t x, int16_t y) {
	_x = x;
	_y = y;
}

void Session::update_position_atomic(int16_t x, int16_t y) {
	// TODO
}

void Session::change_state(S_STATE state) {
	std::lock_guard state_guard{ _state_lock };
	_state = state;
}

void Session::process_recv(int32_t num_bytes, OverExp* ex_over) {
	int remain_data = num_bytes + _prev_remain;
	char* p = ex_over->_send_buf;
	while (remain_data > 0) {
		int packet_size = p[0];
		if (packet_size <= remain_data) {
			process_packet(p);
			p = p + packet_size;
			remain_data = remain_data - packet_size;
		}
		else {
			break;
		}
	}

	_prev_remain = remain_data;
	if (remain_data > 0) {
		memcpy(ex_over->_send_buf, p, remain_data);
	}
}

void Session::process_packet(char* packet) {
	switch (packet[1]) {
	case C2S_P_LOGIN: {
		cs_packet_login* p = reinterpret_cast<cs_packet_login*>(packet);
		if (g_server.is_dummy_client(p->name)) {
			DB_USER_INFO user_info{ rand() % MAP_WIDTH, rand() % MAP_HEIGHT };
			login(p->name, user_info);
			break;
		}

		auto [login_result, user_info] = db_login(_id, p->name);
		if (false == login_result) {
			send_login_fail_packet(LOGIN_FAIL_REASON_INVALID_ID);
			g_server.disconnect(_id);
			break;
		}

		login(p->name, user_info);
		break;
	}

	case C2S_P_MOVE:
	{
		cs_packet_move* p = reinterpret_cast<cs_packet_move*>(packet);
		_last_move_time = p->move_time;
		int16_t x = _x;
		int16_t y = _y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < MAP_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < MAP_WIDTH - 1) x++; break;
		}
		
		if (false == g_server.is_in_map_area(x, y)) {
			break;
		}

		int16_t old_x = _x;
		int16_t old_y = _y;
		_x = x;
		_y = y;

		// update view list & send move packet
		auto [sector_x, sector_y] = g_sector.update_sector(_id, old_x, old_y, _x, _y);

		std::unordered_set<int> near_list;
		_view_list_lock.lock();
		std::unordered_set<int> old_vlist = _view_list;
		_view_list_lock.unlock();

		// 주변 9개 섹터에 대해 검색s
		for (auto dir = 0; dir < DIR_CNT; ++dir) {
			auto [dx, dy] = DIRECTIONS[dir];
			if (false == g_sector.is_valid_sector(sector_x + dx, sector_y + dy)) {
				continue;
			}

			std::lock_guard sector_guard{ g_sector.get_sector_lock(sector_x + dx, sector_y + dy) };
			for (auto cl : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
				auto client = g_server.get_session(cl);
				if (nullptr == client) {
					continue;
				}

				if (client->_state != ST_INGAME) {
					continue;
				}

				if (client->_id == _id) {
					continue;
				}

				if (g_server.can_see(_id, client->_id)) {
					near_list.insert(client->_id);
				}
			}
		}

		send_move_packet(_id);

		for (auto& pl : near_list) {
			auto client = g_server.get_session(pl);
			if (nullptr == client) {
				continue;
			}

			if (g_server.is_pc(pl)) {
				client->_view_list_lock.lock();
				if (client->_view_list.count(_id)) {
					client->_view_list_lock.unlock();
					client->send_move_packet(_id);
				}
				else {
					client->_view_list_lock.unlock();
					client->send_add_player_packet(_id);
				}
			}
			else {
				g_server.wakeup_npc(pl, _id);
			}

			if (0 == old_vlist.count(pl)) {
				send_add_player_packet(pl);
			}
		}

		for (auto& pl : old_vlist) {
			auto client = g_server.get_session(pl);
			if (nullptr == client) {
				continue;
			}

			if (0 == near_list.count(pl)) {
				send_remove_player_packet(pl);
				if (g_server.is_pc(pl)) {
					client->send_remove_player_packet(_id);
				}
			}
		}
	}
    break;

	case C2S_P_ATTACK:
	{
		attack_near_area();
	}
    break;

	default:
		break;
	}
}

void Session::init_npc_name(std::string_view name) {
	sprintf_s(_name, name.data());
	_state = ST_INGAME;
}

void Session::login(std::string_view name, const DB_USER_INFO& user_info) {
    sprintf_s(_name, name.data());

	{
        std::lock_guard state_guard{ _state_lock };
		_x = user_info.x;
		_y = user_info.y;
		_state = ST_INGAME;
	}

	send_login_info_packet();

	// 주변 9개 섹터 모두 검색
	auto [sector_x, sector_y] = g_sector.insert(_id, _x, _y);
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(sector_x + dx, sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(sector_x + dx, sector_y + dy) };
		for (auto cl : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
			auto client = g_server.get_session(cl);
			if (nullptr == client) {
				continue;
			}

			{
				std::lock_guard state_guard(client->_state_lock);
				if (ST_INGAME != client->_state) {
					continue;
				}
			}

			if (client->_id == _id) {
				continue;
			}

			if (false == g_server.can_see(_id, client->_id)) {
				continue;
			}

			if (g_server.is_pc(client->_id)) {
				client->send_add_player_packet(_id);
			}
			else {
				g_server.wakeup_npc(client->_id, _id);
			}

			send_add_player_packet(client->_id);
		}
	}
}

void Session::attack_near_area() {
	_view_list_lock.lock();
	std::unordered_set<int> old_vlist = _view_list;
	_view_list_lock.unlock();

	auto [x, y] = get_position();

	send_chat_packet(SYSTEM_ID, "ATTACK!");
	std::vector<int32_t> attacked_npc{ };
	for (auto cl : old_vlist) {
		auto client = g_server.get_session(cl);
		if (nullptr == client or ST_INGAME != client->get_state() or false == client->is_active()) {
			continue;
		}

		auto [cl_x, cl_y] = client->get_position();
		for (int32_t dir = 0; dir < DIR_CNT; ++dir) {
			auto [dx, dy] = DIRECTIONS[dir];

			auto attack_pos_x = x + dx;
			auto attack_pos_y = y + dy;
			if (attack_pos_x != cl_x or attack_pos_y != cl_y) {
				continue;
			}

			attack(cl);
			break;
		}
	}
}

bool Session::initialize_lua_script() {
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

void Session::update_active_state(bool active) {
	_is_active = active;
}

bool Session::update_active_state_cas(bool& old_state, bool new_state) {
	if (old_state == new_state) {
		return false;
	}

	return std::atomic_compare_exchange_strong(&_is_active, &old_state, new_state);
}

void Session::disconnect() {
	g_sector.erase(_id, _x, _y);
	_view_list_lock.lock();
	std::unordered_set<int32_t> vl = _view_list;
	_view_list_lock.unlock();

	for (auto& p_id : vl) {
		if (g_server.is_npc(p_id)) {
			continue;
		}

		auto pl = g_server.get_session(p_id);
		if (nullptr == pl) {
			continue;
		}

		{
			std::lock_guard state_guard{ pl->_state_lock };
			if (ST_INGAME != pl->_state) {
				continue;
			}
		}

		if (pl->_id == _id) {
			continue;
		}

		pl->send_remove_player_packet(_id);
	}

	::closesocket(_socket);
	change_state(ST_FREE);
}

void Session::do_npc_move(int32_t move_dx, int32_t move_dy) {
	std::unordered_set<int> old_vl;
	// 움직이기 전 섹터에 있던 오브젝트들 모두 추가
	auto [prev_sector_x, prev_sector_y] = g_sector.get_sector_idx(_x, _y);
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(prev_sector_x + dx, prev_sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(prev_sector_x + dx, prev_sector_y + dy) };
		for (auto cl : g_sector.get_sector(prev_sector_x + dx, prev_sector_y + dy)) {
			auto obj = g_server.get_session(cl);
			if (nullptr == obj) {
				continue;
			}

			if (ST_INGAME != obj->_state) {
				continue;
			}

			if (true == g_server.is_npc(obj->_id)) {
				continue;
			}

			if (true == g_server.can_see(_id, obj->_id)) {
				old_vl.insert(obj->_id);
			}
		}
	}

	int32_t x = _x + move_dx;
	int32_t y = _y + move_dy;
	if (false == g_server.is_in_map_area(x, y)) {
		return;
	}

	int32_t old_x = _x;
	int32_t old_y = _y;
	_x = x;
	_y = y;

	// 섹터 변경
	auto [curr_sector_x, curr_sector_y] = g_sector.update_sector(_id, old_x, old_y, x, y);

	// 움직인 후 섹터에 있는 오브젝트들 검사
	std::unordered_set<int> new_vl;
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(curr_sector_x + dx, curr_sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(curr_sector_x + dx, curr_sector_y + dy) };
		for (auto cl : g_sector.get_sector(curr_sector_x + dx, curr_sector_y + dy)) {
			auto obj = g_server.get_session(cl);
			if (nullptr == obj) {
				continue;
			}

			if (ST_INGAME != obj->_state) {
				continue;
			}

			if (true == g_server.is_npc(obj->_id)) {
				continue;
			}

			if (true == g_server.can_see(_id, obj->_id)) {
				new_vl.insert(obj->_id);
			}
		}
	}

	for (auto pl : new_vl) {
		auto client = g_server.get_session(pl);
		if (nullptr == client) {
			continue;
		}

		if (0 == old_vl.count(pl)) {
			// 플레이어의 시야에 등장
			client->send_add_player_packet(_id);
		}
		else {
			// 플레이어가 계속 보고 있음.
			client->send_move_packet(_id);
		}
	}

	for (auto pl : old_vl) {
		auto client = g_server.get_session(pl);
		if (nullptr == client) {
			continue;
		}

		if (0 == new_vl.count(pl)) {
			client->_view_list_lock.lock();
			if (0 != client->_view_list.count(_id)) {
				client->_view_list_lock.unlock();
				client->send_remove_player_packet(_id);
			}
			else {
				client->_view_list_lock.unlock();
			}
		}
	}
}

void Session::do_npc_random_move() {
	int32_t dx = 0;
	int32_t dy = 0;
	switch (rand() % 4) {
	case 0: if (dx < (MAP_WIDTH - 1)) dx++; break;
	case 1: if (dx > 0) dx--; break;
	case 2: if (dy < (MAP_HEIGHT - 1)) dy++; break;
	case 3: if (dy > 0) dy--; break;
	}

	do_npc_move(dx, dy);
}

void Session::process_event_player_move(int32_t target_obj) {
	std::lock_guard lua_guard{ _lua_lock };
	lua_getglobal(_lua_state, "event_player_move");
	lua_pushnumber(_lua_state, target_obj);

	if (LUA_OK != lua_pcall(_lua_state, 1, 0, 0)) {
		std::cout << lua_tostring(_lua_state, -1) << "\n";
		lua_pop(_lua_state, 1);
		return;
	}
}

void Session::process_event_npc_move() {
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

	g_server.add_timer_event(_id, 1s, EV_RANDOM_MOVE, 0);
}

void Session::attack(int32_t client_id) {
	auto session = g_server.get_session(client_id);
	if (nullptr == session) {
		return;
	}

	session->update_hp(-TEMP_ATTACK_DAMAGE);
	if (session->get_hp() <= 0) {
		session->update_active_state(false);

		auto [x, y] = session->get_position();
		g_sector.erase(client_id, x, y);

		send_chat_packet(SYSTEM_ID, std::format("YOU KILLED {}", session->get_name()).c_str());
		send_remove_player_packet(client_id);

		g_server.add_timer_event(client_id, 5s, EV_MONSTER_RESPAWN, SYSTEM_ID);
		return;
	}

	send_chat_packet(client_id, std::format("ATTACKED BY {}", _name).c_str());
	send_attack_packet(client_id);
	if (g_server.is_pc(client_id)) {
		session->send_chat_packet(client_id, std::format("ATTACKED BY {}", _name).c_str());
		send_attack_packet(client_id);
	}
}

void Session::do_recv() {
	DWORD recv_flag{ 0 };
	::memset(&_recv_over._over, 0, sizeof(_recv_over._over));

	_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
	_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;

	::WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag, &_recv_over._over, 0);
}

void Session::do_send(void* packet) {
	OverExp* sdata = new OverExp{ reinterpret_cast<char*>(packet) };
	::WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
}

void Session::send_login_info_packet() {
	sc_packet_avatar_info p;
	p.id = _id;
	p.size = sizeof(sc_packet_avatar_info);
	p.type = S2C_P_AVATAR_INFO;
	p.x = _x;
	p.y = _y;
	p.hp = _hp;
	p.max_hp = 100;
	do_send(&p);
}

void Session::send_login_fail_packet(char reason) {
	sc_packet_login_fail p;
	p.size = sizeof(sc_packet_login_fail);
	p.type = S2C_P_LOGIN_FAIL;
	p.id = _id;
	p.reason = reason;
	do_send(&p);
}

void Session::send_move_packet(int32_t client_id) {
	auto session = g_server.get_session(client_id);
	if (nullptr == session) {
		return;
	}
	
	auto [x, y] = session->get_position();

	sc_packet_move p;
	p.id = client_id;
	p.size = sizeof(sc_packet_move);
	p.type = S2C_P_MOVE;
	p.x = x;
	p.y = y;
	p.move_time = session->_last_move_time;
	do_send(&p);
}

void Session::send_add_player_packet(int32_t client_id) {
	auto session = g_server.get_session(client_id);
	if (nullptr == session) {
		return;
	}

	auto [x, y] = session->get_position();

	sc_packet_enter enter_packet;
	enter_packet.id = client_id;
	strcpy_s(enter_packet.name, session->_name);
	enter_packet.size = sizeof(enter_packet);
	enter_packet.type = S2C_P_ENTER;
	enter_packet.x = x;
	enter_packet.y = y;
	enter_packet.hp = _hp;
	enter_packet.max_hp = 100;

	{
        std::lock_guard view_list_guard{ _view_list_lock };
		_view_list.insert(client_id);
	}

	do_send(&enter_packet);
}

void Session::send_chat_packet(int32_t player_id, const char* message) {
	auto message_len = strlen(message);
	if (message_len > MAX_CHAT_LENGTH - 1) {
		std::cout << std::format("Over max chat length - message len: {}\n", message_len);
		return;
	}

	sc_packet_chat packet;
	packet.id = player_id;
	packet.size = static_cast<char>(sizeof(sc_packet_chat) - MAX_CHAT_LENGTH + message_len + 1);
	packet.type = S2C_P_CHAT;
	std::memset(packet.message, 0, MAX_CHAT_LENGTH);
	std::memcpy(packet.message, message, message_len);
	packet.message[message_len] = 0;

	do_send(&packet);
}

void Session::send_remove_player_packet(int32_t client_id) {
	{
		std::lock_guard view_list_guard{ _view_list_lock };
		if (false == _view_list.contains(client_id)) {
			return;
		}

		_view_list.erase(client_id);
	}

	sc_packet_leave p;
	p.id = client_id;
	p.size = sizeof(p);
	p.type = S2C_P_LEAVE;
	do_send(&p);
}

void Session::send_attack_packet(int32_t target) {
	auto session = g_server.get_session(target);
	if (nullptr == session) {
		return;
	}

	auto hp = session->get_hp();

	sc_packet_attack attack_packet;
	attack_packet.id = target;
	attack_packet.size = sizeof(attack_packet);
	attack_packet.type = S2C_P_ATTACK;
	attack_packet.hp = hp;

	do_send(&attack_packet);
}
