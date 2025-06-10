#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <concurrent_unordered_map.h>
#include <unordered_set>
#include "protocol.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

constexpr int VIEW_RANGE = 5;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
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
class SESSION {
	OVER_EXP _recv_over;

public:
	mutex _s_lock;
	S_STATE _state;
	int _id;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	int		_prev_remain;
	unordered_set <int> _view_list;
	mutex	_vl;
	int		last_move_time;
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET p;
		p.id = _id;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}
	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_remove_player_packet(int c_id)
	{
		_vl.lock();
		if (_view_list.count(c_id))
			_view_list.erase(c_id);
		else {
			_vl.unlock();
			return;
		}
		_vl.unlock();
		SC_REMOVE_PLAYER_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_PLAYER;
		do_send(&p);
	}
};

Concurrency::concurrent_unordered_map<int, std::atomic<std::shared_ptr<SESSION>>> clients;

std::atomic_int g_new_client_id;
SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

bool can_see(int from, int to)
{
	auto from_session = clients.at(from).load();
	auto to_session = clients.at(to).load();
	if (nullptr == from_session or nullptr == to_session) {
		return false;
	}

	if (abs(from_session->x - to_session->x) > VIEW_RANGE) return false;
	return abs(from_session->y - to_session->y) <= VIEW_RANGE;
}

void SESSION::send_move_packet(int c_id)
{
	auto session = clients.at(c_id).load();
	if (nullptr == session) {
		return;
	}

	SC_MOVE_PLAYER_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_PLAYER_PACKET);
	p.type = SC_MOVE_PLAYER;
	p.x = session->x;
	p.y = session->y;
	p.move_time = session->last_move_time;
	do_send(&p);
}

void SESSION::send_add_player_packet(int c_id)
{
	auto session = clients.at(c_id).load();
	if (nullptr == session) {
		return;
	}

	SC_ADD_PLAYER_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, session->_name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_PLAYER;
	add_packet.x = session->x;
	add_packet.y = session->y;
	_vl.lock();
	_view_list.insert(c_id);
	_vl.unlock();
	do_send(&add_packet);
}

void process_packet(int c_id, char* packet)
{
	auto session = clients.at(c_id).load();
	if (nullptr == session) {
		return;
	}

	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(session->_name, p->name);
		{
			lock_guard<mutex> ll{ session->_s_lock };
			session->x = rand() % W_WIDTH;
			session->y = rand() % W_HEIGHT;
			session->_state = ST_INGAME;
		}
		session->send_login_info_packet();
		for (auto& [id, pl] : clients) {
			auto client = pl.load();
			if (nullptr == client) {
				continue;
			}

			{
				lock_guard<mutex> ll(client->_s_lock);
				if (ST_INGAME != client->_state) continue;
			}
			if (client->_id == c_id) continue;
			if (false == can_see(c_id, client->_id))
				continue;
			client->send_add_player_packet(c_id);
			session->send_add_player_packet(client->_id);
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		session->last_move_time = p->move_time;
		short x = session->x;
		short y = session->y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		session->x = x;
		session->y = y;

		unordered_set<int> near_list;
		session->_vl.lock();
		unordered_set<int> old_vlist = session->_view_list;
		session->_vl.unlock();
		for (auto& [id, cl] : clients) {
			auto client = cl.load();
			if (nullptr == client) {
				continue;
			}

			if (client->_state != ST_INGAME) continue;
			if (client->_id == c_id) continue;
			if (can_see(c_id, client->_id))
				near_list.insert(client->_id);
		}

		session->send_move_packet(c_id);

		for (auto& pl : near_list) {
			auto client = clients.at(pl).load();
			if (nullptr == client) {
				continue;
			}

			client->_vl.lock();
			if (client->_view_list.count(c_id)) {
				client->_vl.unlock();
				client->send_move_packet(c_id);
			}
			else {
				client->_vl.unlock();
				client->send_add_player_packet(c_id);
			}

			if (old_vlist.count(pl) == 0)
				session->send_add_player_packet(pl);
		}

		for (auto& pl : old_vlist) {
			auto client = clients.at(pl).load();
			if (nullptr == client) {
				continue;
			}

			if (0 == near_list.count(pl)) {
				session->send_remove_player_packet(pl);
				client->send_remove_player_packet(c_id);
			}
		}
	}
				break;
	}
}

void disconnect(int c_id)
{
	auto session = clients.at(c_id).load();
	if (nullptr == session) {
		return;
	}

	session->_vl.lock();
	unordered_set <int> vl = session->_view_list;
	session->_vl.unlock();
	for (auto& p_id : vl) {
		auto pl = clients.at(p_id).load();
		{
			lock_guard<mutex> ll(pl->_s_lock);
			if (ST_INGAME != pl->_state) continue;
		}
		if (pl->_id == c_id) continue;
		pl->send_remove_player_packet(c_id);
	}
	closesocket(session->_socket);

	lock_guard<mutex> ll(session->_s_lock);
	session->_state = ST_FREE;
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = g_new_client_id++;
			if (client_id != MAX_USER) {
				auto new_session = std::make_shared<SESSION>();
				clients.insert(std::make_pair(client_id, new_session));

				auto session = clients.at(client_id).load();
				if (nullptr == session) {
					break;
				}

				{
					lock_guard<mutex> ll(session->_s_lock);
					session->_state = ST_ALLOC;
				}
				session->x = 0;
				session->y = 0;
				session->_id = client_id;
				session->_name[0] = 0;
				session->_prev_remain = 0;
				session->_socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);
				session->do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			int client_id = static_cast<int>(key);
			auto session = clients.at(client_id).load();
			if (nullptr == session) {
				break;
			}

			int remain_data = num_bytes + session->_prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			session->_prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			session->do_recv();
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		}
	}
}

int main()
{
	HANDLE h_iocp;

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}
