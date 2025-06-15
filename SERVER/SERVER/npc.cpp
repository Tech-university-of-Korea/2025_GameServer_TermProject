#include "pch.h"
#include "npc.h"

Npc::Npc(int32_t id) 
	: ServerObject{ ServerObjectTag::NPC, id } { }

Npc::Npc(ServerObjectTag tag, int32_t id) 
	: ServerObject{ ServerObjectTag::LUA_NPC, id } { }

Npc::~Npc() { }

void Npc::init_npc_name(std::string_view name) {
	std::memcpy(_name, name.data(), name.size());
	_state = ST_INGAME;
}

void Npc::do_npc_move(int32_t move_dx, int32_t move_dy) {
	std::unordered_set<int> old_vl;
	auto [prev_sector_x, prev_sector_y] = g_sector.get_sector_idx(_x, _y);
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(prev_sector_x + dx, prev_sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(prev_sector_x + dx, prev_sector_y + dy) };
		for (auto cl : g_sector.get_sector(prev_sector_x + dx, prev_sector_y + dy)) {
			auto obj = g_server.get_server_object(cl);
			if (nullptr == obj) {
				continue;
			}

			if (ST_INGAME != obj->get_state()) {
				continue;
			}

			auto obj_id = obj->get_id();
			if (true == g_server.is_npc(obj_id)) {
				continue;
			}

			if (true == g_server.can_see(_id, obj_id)) {
				old_vl.insert(obj_id);
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

	std::unordered_set<int> new_vl;
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(curr_sector_x + dx, curr_sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(curr_sector_x + dx, curr_sector_y + dy) };
		for (auto cl : g_sector.get_sector(curr_sector_x + dx, curr_sector_y + dy)) {
			auto obj = g_server.get_server_object(cl);
			if (nullptr == obj) {
				continue;
			}

			if (ST_INGAME != obj->get_state()) {
				continue;
			}

			auto obj_id = obj->get_id();
			if (true == g_server.is_npc(obj_id)) {
				continue;
			}

			if (true == g_server.can_see(_id, obj_id)) {
				new_vl.insert(obj_id);
			}
		}
	}

	for (auto pl : new_vl) {
		auto client = g_server.get_server_object<Session>(pl);
		if (nullptr == client) {
			continue;
		}

		if (false == old_vl.contains(pl)) {
			client->send_enter_packet(_id);
		}
		else {
			client->send_move_packet(_id);
		}
	}

	for (auto pl : old_vl) {
		auto client = g_server.get_server_object<Session>(pl);
		if (nullptr == client) {
			continue;
		}

		if (true == new_vl.contains(pl)) {
			continue;
		}

		client->lock_view_list();
		if (true == client->get_view_list().contains(_id)) {
			client->unlock_view_list();
			client->send_leave_packet(_id);
		}
		else {
			client->unlock_view_list();
		}
	}
}

void Npc::do_npc_random_move() {
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

void Npc::process_event_npc_move() { 
	do_npc_random_move();

	g_server.add_timer_event(_id, 1s, EV_RANDOM_MOVE, 0);
}
