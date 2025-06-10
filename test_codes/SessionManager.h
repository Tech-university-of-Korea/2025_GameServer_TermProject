#pragma once

#include <atomic>
#include <shared_mutex>
#include <concurrent_unordered_map.h>
#include <vector>

#include <WS2tcpip.h>

#include <ranges>

#include "Session.h"
#include "ebr.h"

extern SOCKET g_c_socket;
extern Ebr<Session> g_session_ebr;

template <typename Key = int32_t>
class SessionManager {
public:
    inline constexpr Key MAX_USER = 10000;

public:
    SessionManager() { }
    ~SessionManager() { }

public:
    Session* GetSession(Key key) {
        auto it = m_sessions.find(key);
        if (m_sessions.end() != it) {
            return it.second;
        }

        return nullptr;
    }

    std::vector<Key>& GetNpcIdVector() { 
        return m_npc_ids;
    }

    std::vector<Key>& GetSessionIdVector() { 
        return m_session_ids;
    }

    void OnAccept(HANDLE iocp) { 
        Key key = m_new_client_id.fetch_add(1);
        if (key == MAX_USER) {
            std::cout << std::format("MAX Client Exceeded\n");
            return;
        }
        
        auto new_session = g_session_ebr.PopPointer(key);
        m_sessions.insert(std::make_pair(key, new_session));

        ::CreateIoCompletionPort(iocp, g_c_socket, static_cast<ULONG_PTR>(key), 0);
        new_session->do_recv();

        g_c_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    }

    bool Disconnect(Key key) { 
        auto it = m_sessions.find(key);
        if (m_sessions.end() != it) {
            return false;
        }

        g_session_ebr.PushPointer(it.second);
        m_sessions.at(key) = nullptr;
    }

private:
    std::atomic_int32_t m_new_client_id{ };
    Concurrency::concurrent_unordered_map<Key, Session*> m_sessions{ };

    std::shared_mutex m_session_id_lock{ };
    std::vector<Key> m_session_ids{ };

    std::shared_mutex m_npc_id_lock{ };
    std::vector<Key> m_npc_ids{ };
};