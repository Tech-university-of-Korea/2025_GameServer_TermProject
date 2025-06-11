#include "pch.h"
#include "Console.h"

Console::Console(uint32_t width, uint32_t height, std::string_view name)
    : m_width{ width }, m_height{ height }, m_window_name{ name } {
    m_chat_box = sf::RectangleShape{ sf::Vector2f{ static_cast<float>(m_width), static_cast<float>(m_height) } };
    m_chat_box.setFillColor(sf::Color{ 50, 50, 50, 125 });
    m_chat_box.setPosition(10.0f, SCREEN_HEIGHT * TILE_WIDTH - m_height);
}

Console::~Console() { }

void Console::input(sf::Uint32 unicode) {
    if (unicode >= 128 or unicode == '\r') {
        return;
    }

    m_input += static_cast<char>(unicode);
}

void Console::draw() {
    g_window->draw(m_chat_box);


    g_window->draw(m_input_text);
}