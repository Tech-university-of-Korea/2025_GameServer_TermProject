#pragma once

#include <stdint.h>
#include <atomic>

std::atomic_int32_t g_thread_id_counter{ };

struct thread_local_storage {
    static thread_local int32_t g_thread_id;
};

using tls = thread_local_storage;

void init_thread_local_storage() 
{
    tls::g_thread_id = g_thread_id_counter.fetch_add(1);
}

