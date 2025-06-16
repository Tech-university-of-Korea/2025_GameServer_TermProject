#include "pch.h"
#include "server_entity.h"

ServerEntity::ServerEntity(ServerObjectTag tag, int32_t id) 
	: _tag{ tag }, _id{ id } { 
	std::memset(_name, 0, MAX_ID_LENGTH);
}

ServerEntity::~ServerEntity() { }

bool ServerEntity::is_player() {
	return _id < MAX_USER;
}

bool ServerEntity::is_npc() {
	return _id >= MAX_USER;
}

bool ServerEntity::is_active() {
	return _is_active;
}

int32_t ServerEntity::get_id() {
	return _id;
}

int16_t ServerEntity::get_x() {
	return _x;
}

int16_t ServerEntity::get_y() {
	return _y;
}

std::pair<int16_t, int16_t> ServerEntity::get_position() {
	return std::make_pair(_x, _y);
}

std::string_view ServerEntity::get_name() {
	return _name;
}

int32_t ServerEntity::get_hp() {
	return _hp;
}

int32_t ServerEntity::get_max_hp() {
	return _max_hp;
}

int64_t ServerEntity::get_move_time() {
	return _last_move_time;
}

int32_t ServerEntity::get_level() {
	return _level;
}

int32_t ServerEntity::get_exp() {
	return _exp;
}

int32_t ServerEntity::get_max_exp() {
	return _max_exp;
}

DbUserInfo ServerEntity::get_user_info() {
	DbUserInfo ret;
	ret.x = _x;
	ret.y = _y;
	ret.exp = _exp;
	ret.level = _level;
	return ret;
}

void ServerEntity::init_name(std::string_view name) {
	std::memcpy(_name, name.data(), name.size());
}

void ServerEntity::lock_state() {
	_state_lock.lock();
}

void ServerEntity::unlock_state() {
	_state_lock.unlock();
}

bool ServerEntity::try_respawn(int32_t max_hp) {
	auto old_hp = _hp.load();
	return _hp.compare_exchange_strong(old_hp, max_hp);
}

void ServerEntity::dispatch_event(GameEvent* event, std::chrono::system_clock::duration delay) {
	g_server.add_timer_event(_id, delay, OP_GAME_EVENT, event);
}

void ServerEntity::update_hp(int32_t diff) {
	_hp.fetch_add(diff);

	if (_max_hp < _hp) {
		_hp = _max_hp;
	}
}

ServerObjectTag ServerEntity::get_object_tag() {
	return _tag;
}

std::mutex& ServerEntity::get_state_lock() {
	return _state_lock;
}

ServerObjectState ServerEntity::get_state() {
	return _state;
}

void ServerEntity::update_position(int16_t x, int16_t y) {
	_x = x;
	_y = y;
}

void ServerEntity::update_position_atomic(int16_t x, int16_t y) {
	// TODO
}

void ServerEntity::update_active_state(bool active) {
	_is_active = active;
}

bool ServerEntity::update_active_state_cas(bool& old_state, bool new_state) {
	if (old_state == new_state) {
		return false;
	}

	return std::atomic_compare_exchange_strong(&_is_active, &old_state, new_state);
}

void ServerEntity::update_move_time(int64_t move_time) {
	_last_move_time = move_time;
}

void ServerEntity::change_state(ServerObjectState state) {
	std::lock_guard state_guard{ _state_lock };
	_state = state;
}

void ServerEntity::update_view_list(std::unordered_set<int32_t>& view_list, int32_t sector_x, int32_t sector_y) {
	// 주변 9개 섹터에 대해 검색s
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(sector_x + dx, sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(sector_x + dx, sector_y + dy) };
		for (auto client_id : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
			if (client_id == _id) {
				continue;
			}

			auto client = g_server.get_server_object(client_id);
			if (nullptr == client) {
				continue;
			}

			if (ST_INGAME != client->get_state()) {
				continue;
			}

			if (g_server.can_see(_id, client_id)) {
				view_list.insert(client_id);
			}
		}
	}
}

void ServerEntity::insert_player_view_list(std::unordered_set<int32_t>& view_list, int32_t sector_x, int32_t sector_y) {
	for (auto dir = 0; dir < DIR_CNT; ++dir) {
		auto [dx, dy] = DIRECTIONS[dir];
		if (false == g_sector.is_valid_sector(sector_x + dx, sector_y + dy)) {
			continue;
		}

		std::lock_guard sector_guard{ g_sector.get_sector_lock(sector_x + dx, sector_y + dy) };
		for (auto cl : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
			if (_id == cl) {
				continue;
			}

			auto obj = g_server.get_server_object(cl);
			if (nullptr == obj) {
				continue;
			}

			if (ST_INGAME != obj->get_state()) {
				continue;
			}

			if (true == g_server.is_npc(cl)) {
				continue;
			}

			if (true == g_server.can_see(_id, cl)) {
				view_list.insert(cl);
			}
		}
	}
}
