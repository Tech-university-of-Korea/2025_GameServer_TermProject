#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <deque>
#include <string>
#include <format>

#include "game_header.h"
#include "network_constants.h"
#include "network_modules.h"
#include "Object.h"
#include "hp_bar.h"

using namespace std::literals;

constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

constexpr auto MESSAGE_TIMEOUT = 2000ms;
constexpr auto MAX_CNT_STORED_SYSTEM_MESS = 10;
using MessageDuration = decltype(MESSAGE_TIMEOUT);

struct SystemMessage {
	std::string message;
	std::chrono::system_clock::time_point time;
};

extern int32_t g_left_x;
extern int32_t g_top_y;
extern int32_t g_myid;

extern std::deque<SystemMessage> g_system_messages;
extern sf::RenderWindow* g_window;
extern sf::Font g_font;
extern sf::Text g_system_message;

// draw
extern Object avatar;
extern std::unordered_map<int32_t, Object> players;

extern Object white_tile;
extern Object black_tile;

extern sf::Texture* board;
extern sf::Texture* pieces;
extern std::unique_ptr<HpBar> g_hp_bar;