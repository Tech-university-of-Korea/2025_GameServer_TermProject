#include "pch.h"
#include "session.h"

Session::Session(SOCKET socket, int32_t id) 
	: ServerObject{ ServerObjectTag::SESSION, id }, _socket { socket } { }

Session::~Session() { }

void Session::lock_view_list() {
	_view_list_lock.lock();
}

void Session::unlock_view_list() {
	_view_list_lock.unlock();
}

const std::unordered_set<int32_t>& Session::get_view_list() {
	return _view_list;
}

void Session::process_recv(int32_t num_bytes, OverExp* ex_over) {
	int32_t total_size = num_bytes + _prev_remain;
	if (0 == total_size) return;
	uint8_t* end = reinterpret_cast<uint8_t*>(ex_over->_send_buf) + total_size;
	uint8_t* iter = reinterpret_cast<uint8_t*>(ex_over->_send_buf);
	while (true) {
		uint8_t packet_size = iter[0];
#ifdef _DEBUG // 의심되는 경우만 활성화
		if (0 == packet_size) {
			std::cout << std::format("Fatal Error: Packet Size is zero in Session [{}]\n", _id);
			break;
		}
#endif

        process_packet(iter);
		iter += packet_size;
		if (end <= iter) {
			break;
		}
	}

	_prev_remain = std::distance(iter, end);
	if (_prev_remain > 0) {
		std::memcpy(ex_over->_send_buf, iter, _prev_remain);
	}
}

void Session::process_packet(unsigned char* packet) {
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
		int16_t dx{ };
		int16_t dy{ };
		switch (p->direction) {
		case 0: dy = -1; break;
		case 1: dy = 1; break;
		case 2: dx = -1; break;
		case 3: dx = 1; break;
		}

		do_player_move(dx, dy);
	}
    break;

	case C2S_P_ATTACK:
	{
		cs_packet_attack* p = reinterpret_cast<cs_packet_attack*>(packet);
		attack_near_area();
	}
    break;

	case C2S_P_CHAT:
	{
		cs_packet_chat* p = reinterpret_cast<cs_packet_chat*>(packet);
		g_server.send_chat_packet_to_every_one(_id, p->message);
	}
	break;

	default:
		break;
	}
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
		for (auto entity_id : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
			auto client = g_server.get_server_object(entity_id);
			if (nullptr == client) {
				continue;
			}

			{
				std::lock_guard state_guard{ client->get_state_lock()};
				if (ST_INGAME != client->get_state()) {
					continue;
				}
			}

			if (entity_id == _id) {
				continue;
			}

			if (false == g_server.can_see(_id, entity_id)) {
				continue;
			}

			send_enter_packet(entity_id);
			if (g_server.is_pc(entity_id)) {
				auto player = cast_ptr<Session>(client);
				if (nullptr != player) {
					continue;
				}

                player->send_enter_packet(_id);
			}
			else {
				g_server.wakeup_npc(entity_id, _id);
			}
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
		auto client = g_server.get_server_object(cl);
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

void Session::disconnect() {
	g_sector.erase(_id, _x, _y);

	_view_list_lock.lock();
	std::unordered_set<int32_t> view_list = _view_list;
	_view_list_lock.unlock();

	for (auto& entity_id : view_list) {
		if (entity_id == _id) {
			continue;
		}

		if (g_server.is_npc(entity_id)) {
			continue;
		}

		auto session = g_server.get_server_object<Session>(entity_id);
		if (nullptr == session) {
			continue;
		}

		{
			std::lock_guard state_guard{ session->get_state_lock() };
			if (ST_INGAME != session->get_state()) {
				continue;
			}
		}

		session->send_leave_packet(_id);
	}

	::closesocket(_socket);
	change_state(ST_FREE);
}

void Session::do_player_move(int32_t move_dx, int32_t move_dy) {
	int16_t old_x = _x;
	int16_t old_y = _y;

	if (false == g_server.is_in_map_area(old_x + move_dx, old_y + move_dy)) {
		return;
	}

	_x = old_x + move_dx;
	_y = old_y + move_dy;

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
		for (auto client_id : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
			auto client = g_server.get_server_object(client_id);
			if (nullptr == client) {
				continue;
			}

			if (ST_INGAME != client->get_state()) {
				continue;
			}

			if (client_id == _id) {
				continue;
			}

			if (g_server.can_see(_id, client_id)) {
				near_list.insert(client_id);
			}
		}
	}

	send_move_packet(_id);

	for (auto& pl : near_list) {
		auto client = g_server.get_server_object(pl);
		if (nullptr == client) {
			continue;
		}

		if (g_server.is_pc(pl)) {
			auto player = cast_ptr<Session>(client);
			if (nullptr == player) {
				continue;
			}

			player->_view_list_lock.lock();
			if (player->_view_list.count(_id)) {
				player->_view_list_lock.unlock();
				player->send_move_packet(_id);
			}
			else {
				player->_view_list_lock.unlock();
				player->send_enter_packet(_id);
			}
		}
		else {
			g_server.wakeup_npc(pl, _id);
		}

		if (false == old_vlist.contains(pl)) {
			send_enter_packet(pl);
		}
	}

	for (auto& pl : old_vlist) {
		auto client = g_server.get_server_object(pl);
		if (nullptr == client) {
			continue;
		}

		if (true == near_list.contains(pl)) {
			continue;
		}

		send_leave_packet(pl);
		if (false == g_server.is_pc(pl)) {
			continue;
		}

		auto player = cast_ptr<Session>(client);
		if (nullptr == player) {
			player->send_leave_packet(_id);
		}
	}
}

void Session::attack(int32_t client_id) {
	auto session = g_server.get_server_object(client_id);
	if (nullptr == session) {
		return;
	}

	session->update_hp(-TEMP_ATTACK_DAMAGE);
	if (session->get_hp() <= 0) {
		session->update_active_state(false);

		auto [x, y] = session->get_position();
		g_sector.erase(client_id, x, y);

		send_chat_packet(SYSTEM_ID, std::format("YOU KILLED {}", session->get_name()).c_str());
		send_leave_packet(client_id);

		g_server.add_timer_event(client_id, 5s, EV_MONSTER_RESPAWN, SYSTEM_ID);
		return;
	}

	send_chat_packet(client_id, std::format("ATTACKED BY {}", _name).c_str());
	send_attack_packet(client_id);
	if (g_server.is_pc(client_id)) {
		auto player = cast_ptr<Session>(session);
		if (nullptr == player) {
			return;
		}

		player->send_chat_packet(client_id, std::format("ATTACKED BY {}", _name).c_str());
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
	auto session = g_server.get_server_object(client_id);
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
	p.move_time = session->get_move_time();
	do_send(&p);
}

void Session::send_enter_packet(int32_t client_id) {
	auto session = g_server.get_server_object(client_id);
	if (nullptr == session) {
		return;
	}

	auto [x, y] = session->get_position();

	sc_packet_enter enter_packet;
	enter_packet.id = client_id;
	auto name_view = session->get_name();
	::strcpy_s(enter_packet.name, name_view.data());
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

void Session::send_leave_packet(int32_t client_id) {
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
	auto session = g_server.get_server_object(target);
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
