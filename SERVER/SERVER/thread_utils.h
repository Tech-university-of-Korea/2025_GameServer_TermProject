#pragma once

#include <thread>
#include <atomic>
#include <stdint.h>

inline std::atomic_int32_t g_thread_id{ 0 };
inline thread_local int32_t l_thread_id{ };

void init_tls();
void finish_thread();

enum class ThreadState {
    NONE,
    RUNNING,
    DETACHED,
    TERMINATED
};

class ThreadManager {
public:
    ThreadManager();
    ~ThreadManager();

public:
    size_t get_thread_count() const;

    template <class Callable, typename... Params>
        requires std::is_invocable_v<Callable>
    int32_t create_thread(Callable&& fn, Params&&... params);

    ThreadState get_thread_state(int32_t thread_id);
    bool joinable(int32_t thread_id);
    void join(int32_t thread_id);

private:

private:
    std::mutex _thread_mutex;
    std::vector<std::thread> _threads;
};