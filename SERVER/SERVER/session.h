#pragma once

#include "server_entity.h"

constexpr int32_t TEMP_ATTACK_DAMAGE = 10;

class Session : public ServerObject {
public:
    Session(SOCKET socket, int32_t id);
    virtual ~Session();

public:
    void lock_view_list();
    void unlock_view_list();
    const std::unordered_set<int32_t>& get_view_list();

    virtual bool try_respawn(int32_t max_hp) override;
    virtual void process_game_event(GameEvent* event) override;

    void process_recv(int32_t num_bytes, IoOver* ex_over);
    void process_packet(unsigned char* packet);

    void process_kill_enemy_event(const GameEventKillEnemy* const);

    void login(const DbUserInfo& user_info);
    void attack_near_area();

    void disconnect();

    void do_player_move(int32_t move_dx, int32_t move_dy);

    void attack(int32_t client_id);

	void do_recv();
	void do_send(void* packet);

	void send_login_info_packet();
	void send_login_fail_packet(char reason);
	void send_move_packet(int32_t client_id);
	void send_enter_packet(int32_t client_id);
	void send_chat_packet(int32_t client_id, const char* mess);
    void send_leave_packet(int32_t client_id);
    void send_attack_packet(int32_t target);
    void send_level_up_packet(int32_t client_id, int32_t exp, int32_t max_exp, int32_t level);
    void send_update_exp_packet(int32_t client_id, int32_t exp);

private:
    // NETWORK 
	SOCKET _socket{ INVALID_SOCKET };
    IoOver _recv_over{ };
	int32_t _prev_remain{ };
    
    // VIEW
	std::unordered_set<int> _view_list{ };
	std::mutex _view_list_lock{ };

    // info
    std::mutex _level_lock{ };
};