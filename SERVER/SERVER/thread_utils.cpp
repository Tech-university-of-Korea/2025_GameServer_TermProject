#include "pch.h"
#include "thread_utils.h"

void init_tls() 
{
    l_thread_id = g_thread_id.fetch_add(1);
    std::cout << std::format("initialize tls: thread {}\n", l_thread_id);
}

void finish_thread()
{
    g_session_ebr.clear_thread_epoch();
}