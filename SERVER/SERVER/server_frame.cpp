﻿#include "pch.h"
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

ServerEntity* ServerFrame::get_server_object(int32_t session_id) {
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

bool ServerFrame::can_move(int32_t x, int32_t y) {
	if (x < 0 or x > MAP_WIDTH or y < 0 or y > MAP_HEIGHT) {
		return false;
	}

	auto [sector_x, sector_y] = g_sector.get_sector_idx(x, y);
	decltype(auto) sector = g_sector.get_sector(sector_x, sector_y);
	{
		std::lock_guard sector_guard{ g_sector.get_sector_lock(sector_x, sector_y) };
		for (auto& entity_id : sector) {
			auto entity = get_server_object(entity_id);
			if (nullptr == entity) {
				continue;
			}

			auto [entity_x, entity_y] = entity->get_position();
			if (x == entity_x and y == entity_y) {
				return false;
			}
		}
	}

	return true;
}

void ServerFrame::disconnect(int32_t client_id) {
	auto session = get_server_object<Session>(client_id);
	if (nullptr == session) {
		return;
	}

	auto name_view = session->get_name();
	if (false == is_dummy_client(name_view) or ST_INGAME != session->get_state()) {
		add_db_event(client_id, OP_DB_UPDATE_USER_INFO, name_view, session->get_user_info());
	}

	session->disconnect();
	g_session_ebr.push_ptr(session);
	_sessions.at(client_id) = nullptr;
}

void ServerFrame::add_timer_event(int32_t id, std::chrono::system_clock::duration delay, IoType type, void* extra_info) {
	auto excute_time = std::chrono::system_clock::now() + delay;
	TimerEvent ev{ id, type, extra_info };
	auto insert_pair = std::make_pair(excute_time, ev);

	std::lock_guard timer_map_guard{ _timer_map_lock };
	_timer_event_map.insert(insert_pair);
}

void ServerFrame::add_db_event(int32_t id, IoType type, std::string_view name, std::optional<DbUserInfo> user_info) {
	DataBaseEvent ev{ id, type };
	if (false == name.empty()) {
		::memset(ev.name, 0, MAX_ID_LENGTH);
		::memcpy(ev.name, name.data(), name.length());
	}

	if (std::nullopt != user_info) {
		ev.user_info = user_info.value();
	}

	_db_queue.push(ev);

	_has_db_task.store(true);
	_has_db_task.notify_one();
}

void ServerFrame::wakeup_npc(int32_t npc_id, int32_t waker) {
	auto npc = get_server_object(npc_id);
	if (nullptr == npc) {
		return;
	}

	if (ServerObjectTag::AGRO_NPC == npc->get_object_tag()) {
        add_timer_event(npc_id, 0s, OP_NPC_CHECK_AGRO, reinterpret_cast<void*>(waker));
	}

	if (true == npc->is_active() or npc->get_hp() <= 0) {
		return;
	}

	bool old_state = false;
	if (false == npc->update_active_state_cas(old_state, true)) {
		return;
	}

	add_timer_event(npc_id, NPC_MOVE_TIME, OP_NPC_MOVE);
}

void ServerFrame::send_chat_packet_to_every_one(int32_t sender, std::string_view message) {
	for (auto [id, session] : _sessions) {
		if (is_npc(id) or nullptr == session) {
			continue;
		}
		
		//ServerObject::cast_ptr<Session>(session);
		//session->send_chat_packet(sender, message.data());
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
	::AcceptEx(
		_server_socket, 
		_client_socket, 
		_accept_over._send_buf, 
		0, 
		addr_size + 16, addr_size + 16, 
		0, 
		&_accept_over._over
	);

	std::vector<std::thread> worker_threads{ };
	int32_t num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i) {
		worker_threads.emplace_back([this]() { worker_thread(); });
	}
	std::thread timer_thread{ [this]() { this->timer_thread(); } };
	std::thread db_thread{ [this]() { this->db_thread(); } };

	for (auto& th : worker_threads) {
		th.join();
	}
	timer_thread.join();
	db_thread.join();

	::cleanup_db();
	::closesocket(_server_socket);
	::WSACleanup();
}

void ServerFrame::initialize_npc() {
	std::cout << "NPC intialize begin.\n";
	for (int i = MAX_USER; i < MAX_USER + NUM_MONSTER; ++i) {
		auto random = rand() % 100;
		if (random > 80) {
			auto new_session = new AgroNpc{ i };
			_sessions.insert(std::make_pair(i, new_session));

			auto npc = cast_ptr<AgroNpc>(get_server_object(i));
			npc->init_npc_name();
			npc->update_position(rand() % MAP_WIDTH, rand() % MAP_HEIGHT);

			auto [x, y] = npc->get_position();
			auto _ = g_sector.insert(i, x, y);
            npc->initialize_lua_script();
		}
		else {
			auto new_session = new PeaceNpc{ i };
			_sessions.insert(std::make_pair(i, new_session));

			auto npc = cast_ptr<PeaceNpc>(get_server_object(i));
			npc->init_npc_name();
			npc->update_position(rand() % MAP_WIDTH, rand() % MAP_HEIGHT);

			auto [x, y] = npc->get_position();
			auto _ = g_sector.insert(i, x, y);
		}
	}
	std::cout << "NPC initialize end.\n";
}

void ServerFrame::do_accept() {
	int32_t client_id = _new_client_id++;
	if (client_id != MAX_USER) {
		auto new_session = g_session_ebr.pop_ptr<Session>(_client_socket, client_id);
		_sessions.insert(std::make_pair(client_id, new_session));

		auto session = get_server_object<Session>(client_id);
		if (nullptr == session) {
			return;
		}

		session->change_state(ST_ALLOC);

		::CreateIoCompletionPort(reinterpret_cast<HANDLE>(_client_socket), _iocp_handle, client_id, 0);

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
		BOOL ret = ::GetQueuedCompletionStatus(_iocp_handle, &num_bytes, &key, &over, INFINITE);
		OverExp* ex_over = reinterpret_cast<OverExp*>(over);

		SessionEbrGuard ebr_guard{ g_session_ebr, l_thread_id };

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
			auto io_over = reinterpret_cast<IoOver*>(ex_over);
			int32_t client_id = static_cast<int32_t>(key);
			auto session = get_server_object<Session>(client_id);
			if (nullptr == session) {
				break;
			}

			session->process_recv(num_bytes, io_over);
			session->do_recv();
		}
        break;

		case OP_SEND:
		{
			delete ex_over;
		}
		break;

		case OP_NPC_MOVE:
		case OP_NPC_RESPAWN:
		case OP_NPC_CHECK_AGRO:
		{
			int32_t npc_id = static_cast<int32_t>(key);
			auto npc = get_server_object<Npc>(npc_id);
			if (nullptr == npc) {
				delete ex_over;
				break;
			}

			npc->dispatch_npc_update(ex_over->_comp_type, ex_over->extra_info);
			delete ex_over;
		}
		break;

		case OP_GAME_EVENT:
		{
			int32_t entity_id = static_cast<int32_t>(key);
			auto entity = get_server_object(entity_id);
			if (nullptr == entity) {
				delete ex_over;
				break;
			}

			auto event = reinterpret_cast<GameEvent*>(ex_over->extra_info);
			if (nullptr == event) {
				std::cout << "try process NULL GameEvent!!\n";
			}
			entity->process_game_event(event);

			delete event;
			delete ex_over;
		}
		break;

		case OP_DB_LOGIN:
		{
			int32_t session_id = static_cast<int32_t>(key);
			auto session = get_server_object<Session>(session_id);
			if (nullptr == session) {
				delete ex_over;
				break;
			}

			auto user_info = reinterpret_cast<DbUserInfo*>(ex_over->extra_info);
			session->login(*user_info);

			delete user_info;
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
		auto current_time = std::chrono::system_clock::now();
		_timer_map_lock.lock();
		if (_timer_event_map.empty()) {
			_timer_map_lock.unlock();
			std::this_thread::sleep_for(1ms);
			continue;
		}

		auto beg = _timer_event_map.begin();
		auto end = _timer_event_map.upper_bound(current_time);
		size_t executable_cnt = std::distance(beg, end);
		if (0 == executable_cnt) {
			_timer_map_lock.unlock();
			std::this_thread::sleep_for(1ms);
			continue;
		}

		std::vector<TimerEvent> ev_vec;
		ev_vec.reserve(executable_cnt);

		std::transform(beg, end, std::back_inserter(ev_vec), [](const auto& pair) { return pair.second; }); // 순회를 위한 벡터에 저장
		_timer_event_map.erase(beg, end);
		_timer_map_lock.unlock(); // 더이상의 보호는 불필요

		for (auto& ev : ev_vec) { // 실행할 수 있는건 모두 실행
			OverExp* ov = new OverExp;
			ov->_comp_type = ev.op_type;
			ov->extra_info = ev.extra_info;
			::PostQueuedCompletionStatus(_iocp_handle, 1, ev.obj_id, &ov->_over);
		}

		std::this_thread::sleep_for(1ms); 
	}

	finish_thread();
}

void ServerFrame::db_thread() {
	init_db();
	init_tls();

	while (true) {
		DataBaseEvent ev;
		bool pop_result = _db_queue.try_pop(ev);
		if (false == pop_result) {
            _has_db_task.store(pop_result);
            _has_db_task.wait(false);
			continue;
		}

		SessionEbrGuard guard{ g_session_ebr, l_thread_id };
		switch (ev.event) {
		case OP_DB_LOGIN:
		{
			auto [login_result, user_info] = db_login(ev.obj_id);
			if (false == login_result) {
				DbUserInfo user_info;
				user_info.x = rand() % MAP_WIDTH;
				user_info.y = rand() % MAP_HEIGHT;
				user_info.exp = 0;
				user_info.level = 1;

				if (false == db_create_new_user(ev.obj_id, user_info.x, user_info.y)) {
					g_server.disconnect(ev.obj_id);
					break;
				}

				OverExp* ov = new OverExp;
				ov->_comp_type = OP_DB_LOGIN;
				ov->extra_info = new DbUserInfo{ user_info };
                ::PostQueuedCompletionStatus(_iocp_handle, 1, ev.obj_id, &ov->_over);
			}
			else {
				OverExp* ov = new OverExp;
				ov->_comp_type = OP_DB_LOGIN;
				ov->extra_info = new DbUserInfo{ user_info };
                ::PostQueuedCompletionStatus(_iocp_handle, 1, ev.obj_id, &ov->_over);
			}
		}
		break;

		case OP_DB_UPDATE_USER_INFO:
		{
			db_update_user_info(ev.name, ev.user_info);
		}
		break;

		case OP_DB_CREATE_USER_INFO:
			break;

		default:
			break;
		}
	}

	finish_thread();
	cleanup_db();
}
