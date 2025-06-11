#pragma once

#include "overlapped.h"
#include "db_functions.h"
#include <cstdint>

enum S_STATE { 
    ST_FREE, 
    ST_ALLOC, 
    ST_INGAME 
};

constexpr int32_t TEMP_ATTACK_DAMAGE = 10;

class Session {
public:
    Session(SOCKET socket, int32_t id);
    ~Session();

public:
    bool is_player();
    bool is_npc();
    bool is_active();

    S_STATE get_state();
    int16_t get_x();
    int16_t get_y();
    std::pair<int16_t, int16_t> get_position();
    std::string_view get_name();
    int32_t get_hp();

    bool try_respawn(int32_t max_hp);
    void update_hp(int32_t diff);

    void update_position(int16_t x, int16_t y);
    void update_position_atomic(int16_t x, int16_t y);

    void change_state(S_STATE state);

    void process_recv(int32_t num_bytes, OverExp* ex_over);
    void process_packet(char* packet);

    void init_npc_name(std::string_view name);
    void login(std::string_view name, const DB_USER_INFO& user_info);
    void attack_near_area();
    bool initialize_lua_script();

    void update_active_state(bool active);
    bool update_active_state_cas(bool& old_state, bool new_state);
    void disconnect();

    void do_npc_move(int32_t move_dx, int32_t move_dy);
    void do_npc_random_move();

    void process_event_player_move(int32_t target_obj);
    void process_event_npc_move();

    void attack(int32_t client_id);

	void do_recv();
	void do_send(void* packet);
	void send_login_info_packet();
	void send_login_fail_packet(char reason);
	void send_move_packet(int32_t client_id);
	void send_add_player_packet(int32_t client_id);
	void send_chat_packet(int32_t client_id, const char* mess);
    void send_remove_player_packet(int32_t client_id);
    void send_attack_packet(int32_t target);

public:
    // EBR
    uint64_t _epoch_counter{ };

private:
    // SESSION's STATE
    std::mutex _state_lock{ };
    S_STATE _state{ ST_FREE };

    // INGAME INFORMATION
    char _name[MAX_ID_LENGTH]{ };
    int16_t _x{ 0 };
    int16_t _y{ 0 };
    int32_t _level{ 1 };
    int32_t _exp{ 0 };
    std::atomic_int32_t _hp{ 100 };

    // NETWORK 
	SOCKET _socket{ INVALID_SOCKET };

	int32_t _id{ SYSTEM_ID };
	OverExp _recv_over{ };
	int32_t _prev_remain{ };
    
    // VIEW
	std::unordered_set<int> _view_list{ };
	std::mutex _view_list_lock{ };
	int32_t _last_move_time{ 0 };

    // FOR NPC
    // LUA SCRIPT
    std::mutex _lua_lock{ };
    lua_State* _lua_state{ nullptr };

    std::atomic_bool _is_active{ false };
};