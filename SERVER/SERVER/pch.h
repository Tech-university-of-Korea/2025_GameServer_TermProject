#pragma once

#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <chrono>
#include <concurrent_priority_queue.h>
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <atomic>

#include "db_util.h"
#include "game_header.h"
#include "sector.h"

#include "include/lua.hpp"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")

#include "server_frame.h"
#include "sector.h"
#include "network_constants.h"
#include "thread_utils.h"
#include "ebr.h"

using namespace std::literals;

extern ServerFrame g_server;
extern Sectors g_sector;
extern EBR<Session> g_session_ebr;