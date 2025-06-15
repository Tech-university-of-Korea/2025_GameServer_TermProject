#pragma once

#include <thread>

class Console {
public:
    Console();
    ~Console();

public:

private:
    void chat_thread();

private:
    std::thread m_chat_thread{ };
};