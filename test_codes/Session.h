#pragma once

#include <WS2tcpip.h>

#include <Windows.h>

#include <chrono>
#include <unordered_set>
#include <atomic>
#include <mutex>

#include "protocol.h"

constexpr int VIEW_RANGE = 5;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	int _ai_target_obj;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };

class Session {
public:
	Session()
	{
		m_is_active = false;
		m_id = -1;
		m_socket = 0;
		x = y = 0;
		m_name[0] = 0;
		m_state = ST_FREE;
		m_prev_remain = 0;
	}

	Session(SOCKET socket, int32_t id) : m_socket{ socket }, m_id{ id } {
		m_is_active = false;
		m_socket = INVALID_SOCKET;
		x = y = 0;
		m_name[0] = 0;
		m_state = ST_FREE;
		m_prev_remain = 0;
		m_last_move_time = 0;
	}

	~Session() {}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&m_recv_over._over, 0, sizeof(m_recv_over._over));
		m_recv_over._wsabuf.len = BUF_SIZE - m_prev_remain;
		m_recv_over._wsabuf.buf = m_recv_over._send_buf + m_prev_remain;
		WSARecv(m_socket, &m_recv_over._wsabuf, 1, 0, &recv_flag,
			&m_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(m_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET p;
		p.id = m_id;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}
	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_chat_packet(int c_id, const char* mess);
	void send_remove_player_packet(int c_id)
	{
		m_view_list_lock.lock();
		if (m_view_list.count(c_id)) {
			m_view_list.erase(c_id);
		}
		else {
			m_view_list_lock.unlock();
			return;
		}
		m_view_list_lock.unlock();
		SC_REMOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_OBJECT;
		do_send(&p);
	}
	void wakeup();

private:
	OVER_EXP m_recv_over;

public:
	std::atomic_bool m_is_active;
	std::mutex _s_lock;
	S_STATE m_state;
	int32_t	m_prev_remain;

	int m_id;
	SOCKET m_socket;
	short x;
	short y;

	char	m_name[NAME_SIZE];

	std::unordered_set <int> m_view_list;
	std::mutex	m_view_list_lock;
	long long	m_last_move_time;
};

