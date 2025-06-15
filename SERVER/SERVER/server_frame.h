#pragma once

#include "session.h"
#include "lua_npc.h"
#include "overlapped.h"

using SessionPtr = Session*;

enum DB_EVENT_TYPE { 
    DB_LOGIN, 
    DB_UPDATE_USER_INFO
};

struct TimerEvent {
    int32_t obj_id;
    std::chrono::system_clock::time_point wakeup_time;
    COMP_TYPE op_type;
    void* extra_info;

    constexpr bool operator<(const TimerEvent& other) const {
        return wakeup_time > other.wakeup_time;
    }
};

struct DB_EVENT {
    int32_t id;
    DB_EVENT_TYPE event;
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

    void disconnect(int32_t client_id);
    void add_timer_event(int32_t id, std::chrono::system_clock::duration delay, COMP_TYPE type, void* extra_info=nullptr);
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

    OverExp _accept_over{ };

    std::atomic_int32_t _new_client_id{ 0 };
    Concurrency::concurrent_unordered_map<int32_t, ServerObject*> _sessions{ };

    std::mutex _timer_queue_lock{ };
    concurrency::concurrent_priority_queue<TimerEvent> _timer_queue;
    //concurrency::concurrent_queue<DB_EVENT> _db_queue;
};