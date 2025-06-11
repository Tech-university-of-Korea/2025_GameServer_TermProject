#pragma once

class Console {
public:
    Console(uint32_t width, uint32_t height, std::string_view name);
    ~Console();

public:
    void input(sf::Uint32 unicode);
    void draw();

private:
    std::string m_window_name{ };
    uint32_t m_width{ };
    uint32_t m_height{ };
    sf::RectangleShape m_chat_box{ };
    std::string m_input{ };
    sf::Text m_input_text{ };
};