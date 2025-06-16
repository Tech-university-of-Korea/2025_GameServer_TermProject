#include "pch.h"
#include "npc.h"

Npc::Npc(ServerObjectTag tag, int32_t id) 
	: ServerEntity{ tag, id } { }

Npc::~Npc() { }

void Npc::do_npc_move(int32_t move_dx, int32_t move_dy) {
	int32_t x = _x + move_dx;
	int32_t y = _y + move_dy;
	if (false == g_server.can_move(x, y)) {
		return;
	}

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

	int32_t old_x = _x;
	int32_t old_y = _y;
	_x = x;
	_y = y;

	// 섹터 변경
	auto [curr_sector_x, curr_sector_y] = g_sector.update_sector(_id, old_x, old_y, x, y);

	std::unordered_set<int32_t> new_vl;
	insert_player_view_list(new_vl, curr_sector_x, curr_sector_y);

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

bool Npc::check_player_in_view_range() {
	auto [x, y] = get_position();
	auto [sector_x, sector_y] = g_sector.get_sector_idx(x, y);
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(sector_x + dx, sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(sector_x + dx, sector_y + dy) };
		for (auto entity_id : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
			if (g_server.is_npc(entity_id)) {
				continue;
			}

			auto entity = g_server.get_server_object(entity_id);
			if (nullptr != entity and ST_INGAME != entity->get_state()) {
				continue;
			}

			if (g_server.can_see(_id, entity_id)) {
				return true;
			}
		}
	}

	return false;
}

void Npc::chase_target() {
	auto player = g_server.get_server_object<Session>(_target_player);
	if (nullptr == player) {
		_target_player = SYSTEM_ID;
		return;
	}

	auto [x, y] = player->get_position();
	auto diff_x = x - _x;
	auto diff_y = y - _y;
	if (1 >= std::abs(diff_x) + std::abs(diff_y)) {
		attack();
		return;
	}
	else if (1 == std::abs(diff_x) and 1 == std::abs(diff_y)) {
		do_npc_move(1, 0);
		return;
	}
	
	auto dx = 0 == diff_x ? 0 : diff_x / std::abs(diff_x);
	auto dy = 0 == diff_y ? 0 : diff_y / std::abs(diff_y);
	do_npc_move(dx, dy);
}

void Npc::attack() {
    std::unordered_set<int> view_list;
	auto [x, y] = get_position();
	auto [sector_x, sector_y] = g_sector.get_sector_idx(x, y);

	insert_player_view_list(view_list, sector_x, sector_y);

    for (auto entity_id : view_list) {
        auto session = g_server.get_server_object<Session>(entity_id);
        if (nullptr == session or ST_INGAME != session->get_state() or false == session->is_active()) {
            continue;
        }

        auto [cl_x, cl_y] = session->get_position();
        for (int32_t dir = 0; dir < DIR_CNT; ++dir) {
            auto [dx, dy] = DIRECTIONS[dir];

            auto attack_pos_x = x + dx;
            auto attack_pos_y = y + dy;
            if (attack_pos_x != cl_x or attack_pos_y != cl_y) {
                continue;
            }

			auto damage = rand() % ATTACK_DAMAGE_RANGE + _level;
			session->dispatch_game_event<GameEventGetDamage>(_id, 0s, damage);
            break;
        }
    }
}

void Npc::process_event_npc_move() {
	if (_target_player == SYSTEM_ID or _target_player >= MAX_USER) {
		_target_player = SYSTEM_ID;
        do_npc_random_move();
	}
	else {
		chase_target();
	}

	g_server.add_timer_event(_id, NPC_MOVE_TIME, OP_NPC_MOVE);
}

void Npc::process_game_event(GameEvent* event) {
	auto type = event->event_type;
	switch (type) {
	case GameEventType::EVENT_GET_DAMAGE:
	{
		auto damage_ev = cast_event<GameEventGetDamage>(event);
		update_hp(-damage_ev->damage);
		_target_player = damage_ev->sender;

		auto [x, y] = get_position();
		auto [sector_x, sector_y] = g_sector.get_sector_idx(x, y);

		std::unordered_set<int32_t> new_vl;
		insert_player_view_list(new_vl, sector_x, sector_y);
		for (auto pl : new_vl) {
			auto client = g_server.get_server_object<Session>(pl);
			if (nullptr == client) {
				continue;
			}

            client->send_stat_change_packet(_id);
			if (_hp <= 0) {
				client->send_leave_packet(_id);
			}
		}

		if (_hp <= 0) {
            update_active_state(false);
            g_sector.erase(_id, x, y);
            g_server.add_timer_event(_id, NPC_RESPAWN_TIME, OP_NPC_RESPAWN);
			if (g_server.is_pc(damage_ev->sender)) {
				auto player = g_server.get_server_object<Session>(damage_ev->sender);
				if (nullptr == player) {
					break;
				}

				player->dispatch_game_event<GameEventKillEnemy>(_id, 0s, _level * _level * 2);
			}
		}
	}
    break;

	case GameEventType::EVENT_KILL_ENEMY:
	{
		if (_target_player == event->sender) {
			_target_player = SYSTEM_ID;
			break;
		}
	}
	break;

	default:
		break;
	}
}

void Npc::dispatch_npc_update(IoType type, void* extra_info) {
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

	default:
		break;
	}
}
