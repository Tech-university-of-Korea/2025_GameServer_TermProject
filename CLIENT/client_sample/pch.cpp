#include "pch.h"

int32_t g_left_x;
int32_t g_top_y;
int32_t g_myid;

std::deque<SystemMessage> g_system_messages;
sf::RenderWindow* g_window;
sf::Font g_font;
sf::Text g_system_message;

Object avatar;
std::unordered_map<int32_t, Object> players;

Object white_tile;
Object black_tile;

sf::Texture* board;
sf::Texture* pieces;
std::unique_ptr<HpBar> g_hp_bar;