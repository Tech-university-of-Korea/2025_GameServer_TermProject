#pragma once

#include "session.h"
#include "overlapped.h"

using SessionPtr = std::shared_ptr<Session>;

enum TIMER_EVENT_TYPE { EV_RANDOM_MOVE };
enum DB_EVENT_TYPE { 
    DB_LOGIN, 
    DB_UPDATE_USER_INFO
};

struct TIMER_EVENT {
    int32_t obj_id;
    std::chrono::system_clock::time_point wakeup_time;
    TIMER_EVENT_TYPE event_id;
    int32_t target_id;

    constexpr bool operator<(const TIMER_EVENT& other) const {
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

    SessionPtr get_session(int32_t session_id);
    bool can_see(int32_t from, int32_t to);

    void disconnect(int32_t client_id);
    void add_timer_event(int32_t id, std::chrono::system_clock::duration delay, TIMER_EVENT_TYPE type, int32_t target_id);
    void wakeup_npc(int32_t npc_id, int32_t waker);

    void run();

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
    Concurrency::concurrent_unordered_map<int32_t, std::atomic<SessionPtr>> _sessions{ };
    concurrency::concurrent_priority_queue<TIMER_EVENT> _timer_queue;
    //concurrency::concurrent_queue<DB_EVENT> _db_queue;
};