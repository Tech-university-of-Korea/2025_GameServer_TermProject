#include "pch.h"
#include "server_frame.h"

bool ServerFrame::is_pc(int32_t id) {
	return id < MAX_USER;
}

bool ServerFrame::is_npc(int32_t id) {
	return id >= MAX_USER;
}

bool ServerFrame::is_dummy_client(std::string_view name) {
	std::string s_name{ name };
	if ("Dummy" == s_name.substr(0, 5)) {
		return true;
	}

	return false;
}

bool ServerFrame::is_in_map_area(int16_t x, int16_t y) {
	return (x > 0 and x < MAP_WIDTH and y > 0 and y < MAP_HEIGHT);
}

SessionPtr ServerFrame::get_session(int32_t session_id) {
    return _sessions.at(session_id);
}

bool ServerFrame::can_see(int32_t from, int32_t to) {
    auto from_session = _sessions.at(from);
    auto to_session = _sessions.at(to);
    if (nullptr == from_session or nullptr == to_session) {
        return false;
    }

    auto [from_x, from_y] = from_session->get_position();
    auto [to_x, to_y] = to_session->get_position();

    if (abs(from_x - to_x) > VIEW_RANGE) {
        return false;
    }

    return abs(from_y - to_y) <= VIEW_RANGE;
}

void ServerFrame::disconnect(int32_t client_id) {
	auto session = get_session(client_id);
	if (nullptr == session) {
		return;
	}

	db_update_user_pos(client_id);
	session->disconnect();

	g_session_ebr.push_ptr(session);
	_sessions.at(client_id) = nullptr;
}

void ServerFrame::add_timer_event(int32_t id, std::chrono::system_clock::duration delay, TIMER_EVENT_TYPE type, int32_t target_id) {
	TIMER_EVENT ev{ id, std::chrono::system_clock::now() + delay, type, target_id };
	_timer_queue.push(ev);
}

void ServerFrame::wakeup_npc(int32_t npc_id, int32_t waker) {
	auto npc = get_session(npc_id);
	if (nullptr == npc) {
		return;
	}

	OverExp* exover = new OverExp;
	exover->_comp_type = OP_AI_HELLO;
	exover->_ai_target_obj = waker;
	::PostQueuedCompletionStatus(_iocp_handle, 1, npc_id, &exover->_over);

	if (true == npc->is_active() or npc->get_hp() <= 0) {
		return;
	}

	bool old_state = false;
	if (false == npc->update_active_state_cas(old_state, true)) {
		return;
	}

	add_timer_event(npc_id, 1s, EV_RANDOM_MOVE, 0);
}

void ServerFrame::send_chat_packet_to_every_one(int32_t sender, std::string_view message) {
	for (auto [id, session] : _sessions) {
		if (is_npc(id) or nullptr == session) {
			continue;
		}
		
		session->send_chat_packet(sender, message.data());
	}
}

void ServerFrame::run() {
	WSADATA WSAData;
	if (0 != ::WSAStartup(MAKEWORD(2, 2), &WSAData)) {
		std::cout << "WSAStartup Failure\n";
	}

	_server_socket = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	sockaddr_in server_addr;
	::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(GAME_PORT);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(_server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	::listen(_server_socket, SOMAXCONN);
	sockaddr_in cl_addr;
	int32_t addr_size = sizeof(cl_addr);

	init_db();
	initialize_npc();

	_iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	::CreateIoCompletionPort(reinterpret_cast<HANDLE>(_server_socket), _iocp_handle, 9999, 0);
	_client_socket = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	_accept_over._comp_type = OP_ACCEPT;
	::AcceptEx(_server_socket, _client_socket, _accept_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &_accept_over._over);

	std::vector<std::thread> worker_threads{ };
	int32_t num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i) {
		worker_threads.emplace_back([this]() { worker_thread(); });
	}

	std::thread timer_thread{ [this]() { this->timer_thread(); } };
	timer_thread.join();

	for (auto& th : worker_threads) {
		th.join();
	}

	::cleanup_db();
	::closesocket(_server_socket);
	::WSACleanup();
}

void ServerFrame::initialize_npc() {
	std::cout << "NPC intialize begin.\n";
	for (int i = MAX_USER; i < MAX_USER + NUM_MONSTER; ++i) {
		auto new_session = new Session{ INVALID_SOCKET, i };
		_sessions.insert(std::make_pair(i, new_session));

		auto npc = get_session(i);
		npc->init_npc_name(std::format("NPC{}", i));
        npc->update_position(rand() % MAP_WIDTH, rand() % MAP_HEIGHT);

		auto [x, y] = npc->get_position();
		auto _ = g_sector.insert(i, x, y);

		npc->initialize_lua_script();
	}
	std::cout << "NPC initialize end.\n";
}

void ServerFrame::do_accept() {
	int32_t client_id = _new_client_id++;
	if (client_id != MAX_USER) {
		auto new_session = g_session_ebr.pop_ptr(_client_socket, client_id);
		_sessions.insert(std::make_pair(client_id, new_session));

		auto session = get_session(client_id);
		if (nullptr == session) {
			return;
		}

		session->change_state(ST_ALLOC);

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(_client_socket), _iocp_handle, client_id, 0);

		session->do_recv();
		_client_socket = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	}
	else {
		std::cout << "Max user exceeded.\n";
	}

	::ZeroMemory(&_accept_over._over, sizeof(_accept_over._over));
	int32_t addr_size = sizeof(SOCKADDR_IN);

	::AcceptEx(
		_server_socket, 
		_client_socket, 
		_accept_over._send_buf, 
		0, 
		addr_size + 16, addr_size + 16,
		0, 
		&_accept_over._over
	);
}

void ServerFrame::worker_thread() {
	init_tls();

	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(_iocp_handle, &num_bytes, &key, &over, INFINITE);
		OverExp* ex_over = reinterpret_cast<OverExp*>(over);

		EBRGuard ebr_guard{ g_session_ebr, l_thread_id };

		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) {
				std::cout << "Accept Error";
			}
			else {
				std::cout << "GQCS Error on client[" << key << "]\n";

				disconnect(static_cast<int32_t>(key));
				if (ex_over->_comp_type == OP_SEND) {
					delete ex_over;
				}

				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int32_t>(key));
			if (ex_over->_comp_type == OP_SEND) {
				delete ex_over;
			}

			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: 
		{
			do_accept();
		}
		break;

		case OP_RECV: 
		{
			int32_t client_id = static_cast<int32_t>(key);
			auto session = get_session(client_id);
			if (nullptr == session) {
				break;
			}

			session->process_recv(num_bytes, ex_over);
			session->do_recv();
		}
        break;

		case OP_SEND:
		{
			delete ex_over;
		}
		break;

		case OP_NPC_MOVE: 
		{
			int32_t npc_id = static_cast<int32_t>(key);
			auto npc = get_session(npc_id);
			if (nullptr == npc) {
				delete ex_over;
				break;
			}

			if (false == npc->is_active()) {
				delete ex_over;
				break;
			}

			bool keep_alive = false;
			auto [npc_x, npc_y] = npc->get_position();
			auto [sector_x, sector_y] = g_sector.get_sector_idx(npc_x, npc_y);
			for (auto dir = 0; dir < DIR_CNT; ++dir) {
				auto [dx, dy] = DIRECTIONS[dir];
				if (false == g_sector.is_valid_sector(sector_x + dx, sector_y + dy)) {
					continue;
				}

				std::lock_guard sector_guard{ g_sector.get_sector_lock(sector_x + dx, sector_y + dy) };
				for (auto cl : g_sector.get_sector(sector_x + dx, sector_y + dy)) {
					if (is_npc(cl)) {
						continue;
					}

					auto client = get_session(cl);
					if (nullptr != client and ST_INGAME != client->get_state()) {
						continue;
					}

					if (can_see(npc_id, cl)) {
						keep_alive = true;
						break;
					}
				}

				if (true == keep_alive) {
					break;
				}
			}

			if (true == keep_alive) {
				npc->process_event_npc_move();
			}
			else {
				npc->update_active_state(false);
			}
			delete ex_over;
		}
        break;

		case OP_NPC_RESPAWN:
		{
			int32_t npc_id = static_cast<int32_t>(key);
			auto npc = get_session(npc_id);
			if (nullptr == npc) {
				delete ex_over;
				break;
			}

			std::cout << std::format("Respawn npc: {}\n", npc_id);
			if (true == npc->try_respawn(100)) {
				auto [x, y] = npc->get_position();
				g_sector.insert(npc_id, x, y);
			}
			delete ex_over;
		}
		break;

		case OP_AI_HELLO: 
		{
			int32_t npc_id = static_cast<int32_t>(key);
			auto npc = get_session(npc_id);
			if (nullptr == npc) {
				delete ex_over;
				break;
			}

			npc->process_event_player_move(ex_over->_ai_target_obj);
			delete ex_over;
		}
        break;

		default:
			std::cout << "ERROR: INVALID IO_OP\n";
			break;
		}
	}

	finish_thread();
}

void ServerFrame::timer_thread() {
	init_tls();

	while (true) {
		// 필요할 경우 EBR 적용
		// 현재는 timer_thread에서 session 객체 사용 X -> EBR 적용 필요 X
		
		TIMER_EVENT ev;
		auto current_time = std::chrono::system_clock::now();
		if (false == _timer_queue.try_pop(ev)) {
            std::this_thread::sleep_for(1ms);
			continue;
		}

        if (ev.wakeup_time > current_time) {
            _timer_queue.push(ev);

            std::this_thread::sleep_for(1ms);
            continue;
        }

        switch (ev.event_id) {
        case EV_RANDOM_MOVE:
        {
            OverExp* ov = new OverExp;
            ov->_comp_type = OP_NPC_MOVE;
            ::PostQueuedCompletionStatus(_iocp_handle, 1, ev.obj_id, &ov->_over);
        }
		break;

		case EV_MONSTER_RESPAWN:
		{
			OverExp* ov = new OverExp;
			ov->_comp_type = OP_NPC_RESPAWN;
			::PostQueuedCompletionStatus(_iocp_handle, 1, ev.obj_id, &ov->_over);
		}
        break;

        default:
            break;
        }
	}

	finish_thread();
}

void ServerFrame::db_thread() {
}

