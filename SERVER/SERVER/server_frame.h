#pragma once

#include "session.h"
#include "lua_npc.h"
#include "overlapped.h"

#include <map>
#include <queue>

using SessionPtr = Session*;

struct TimerEvent {
    int32_t obj_id;
    IoType op_type;
    void* extra_info;
};

struct DataBaseEvent {
    int32_t obj_id;
    IoType event;
    char name[MAX_ID_LENGTH]; // for disconnect
    DbUserInfo user_info;
};

class ServerFrame {
public:
    ServerFrame() = default;
    ~ServerFrame() = default;
    
public:
    bool is_pc(int32_t id);
    bool is_npc(int32_t id);
    bool is_dummy_client(std::string_view name);
    bool is_in_map_area(int16_t x, int16_t y);

    ServerObject* get_server_object(int32_t session_id);
    bool can_see(int32_t from, int32_t to);
    bool can_move(int32_t x, int32_t y);

    void disconnect(int32_t client_id);
    void add_timer_event(int32_t id, std::chrono::system_clock::duration delay, IoType type, void* extra_info=nullptr);
    void add_db_event(int32_t id, IoType type, std::string_view name = std::string_view{ }, std::optional<DbUserInfo> user_info = std::nullopt);
    void wakeup_npc(int32_t npc_id, int32_t waker);

    void send_chat_packet_to_every_one(int32_t sender, std::string_view message);

    void run();

public:
    template <typename ObjectType> 
        requires std::derived_from<ObjectType, ServerObject>
    ObjectType* get_server_object(int32_t session_id) {
        ServerObject* obj = get_server_object(session_id);
        return cast_ptr<ObjectType>(obj);
    }

private:
    void initialize_npc();

    void do_accept();

    void worker_thread();
    void timer_thread();
    void db_thread();

private:
    HANDLE _iocp_handle{ INVALID_HANDLE_VALUE };

    SOCKET _server_socket{ INVALID_SOCKET };
    SOCKET _client_socket{ INVALID_SOCKET };

    IoOver _accept_over{ };

    std::atomic_int32_t _new_client_id{ 0 };
    Concurrency::concurrent_unordered_map<int32_t, ServerObject*> _sessions{ };

    std::mutex _timer_map_lock;
    std::multimap<std::chrono::system_clock::time_point, TimerEvent> _timer_event_map;

    std::atomic_bool _has_db_task;
    concurrency::concurrent_queue<DataBaseEvent> _db_queue;
};