#pragma once

#include <thread>
#include <atomic>
#include <stdint.h>

std::atomic_int32_t g_thread_id{ 0 };
thread_local int32_t l_thread_id{ };

void init_tls();