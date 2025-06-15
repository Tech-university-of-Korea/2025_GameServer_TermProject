#include "pch.h"
#include "server_entity.h"

ServerObject::ServerObject(ServerObjectTag tag, int32_t id) 
	: _tag{ tag }, _id{ id } { 
	std::memset(_name, 0, MAX_ID_LENGTH);
}

ServerObject::~ServerObject() { }

bool ServerObject::is_player() {
	return _id < MAX_USER;
}

bool ServerObject::is_npc() {
	return _id >= MAX_USER;
}

bool ServerObject::is_active() {
	return _is_active;
}

int32_t ServerObject::get_id() {
	return _id;
}

int16_t ServerObject::get_x() {
	return _x;
}

int16_t ServerObject::get_y() {
	return _y;
}

std::pair<int16_t, int16_t> ServerObject::get_position() {
	return std::make_pair(_x, _y);
}

std::string_view ServerObject::get_name() {
	return _name;
}

int32_t ServerObject::get_hp() {
	return _hp;
}

int64_t ServerObject::get_move_time() {
	return _last_move_time;
}

void ServerObject::lock_state() {
	_state_lock.lock();
}

void ServerObject::unlock_state() {
	_state_lock.unlock();
}

bool ServerObject::try_respawn(int32_t max_hp) {
	auto old_hp = _hp.load();
	return _hp.compare_exchange_strong(old_hp, max_hp);
}

void ServerObject::update_hp(int32_t diff) {
	_hp.fetch_add(diff);
}

ServerObjectTag ServerObject::get_object_tag() {
	return _tag;
}

std::mutex& ServerObject::get_state_lock() {
	return _state_lock;
}

ServerObjectState ServerObject::get_state() {
	return _state;
}

void ServerObject::update_position(int16_t x, int16_t y) {
	_x = x;
	_y = y;
}

void ServerObject::update_position_atomic(int16_t x, int16_t y) {
	// TODO
}

void ServerObject::update_active_state(bool active) {
	_is_active = active;
}

bool ServerObject::update_active_state_cas(bool& old_state, bool new_state) {
	if (old_state == new_state) {
		return false;
	}

	return std::atomic_compare_exchange_strong(&_is_active, &old_state, new_state);
}

void ServerObject::update_move_time(int64_t move_time) {
	_last_move_time = move_time;
}

void ServerObject::change_state(ServerObjectState state) {
	std::lock_guard state_guard{ _state_lock };
	_state = state;
}
