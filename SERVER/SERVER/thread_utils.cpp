#include "pch.h"
#include "thread_utils.h"

void init_tls() 
{
    l_thread_id = g_thread_id.fetch_add(1);
}
