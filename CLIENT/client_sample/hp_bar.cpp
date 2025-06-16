#include "pch.h"
#include "hp_bar.h"

void HpBar::initialize_texture() {
    m_bar_bg_tex = new sf::Texture{ };
    m_hp_bar_tex = new sf::Texture{ };
    m_exp_bar_tex = new sf::Texture{ };

    m_bar_bg_tex->loadFromFile("assets/bar_bg.png");
    m_hp_bar_tex->loadFromFile("assets/hp_bar.png");
    m_exp_bar_tex->loadFromFile("assets/exp_bar.png");;
}

void HpBar::destroy_texture() {
    if (nullptr != m_bar_bg_tex) {
        delete m_bar_bg_tex;
        m_bar_bg_tex = nullptr;
    }

    if (nullptr != m_hp_bar_tex) {
        delete m_hp_bar_tex;
        m_hp_bar_tex = nullptr;
    }

    if (nullptr != m_exp_bar_tex) {
        delete m_exp_bar_tex;
        m_exp_bar_tex = nullptr;
    }
}

HpBar::HpBar() {
    m_hp_bar.setTexture(*m_hp_bar_tex);
    m_hp_bar.setTextureRect(sf::IntRect(0, 0, 277, 13));
    m_exp_bar.setTexture(*m_exp_bar_tex);
    m_exp_bar.setTextureRect(sf::IntRect(0, 0, 277, 13));
    m_bar_bg.setTexture(*m_bar_bg_tex);
    m_bar_bg.setTextureRect(sf::IntRect(0, 0, 286, 17));
}

HpBar::~HpBar() { }

void HpBar::draw_hp_bar(int32_t x, int32_t y, int32_t hp, int32_t max_hp, float scale, bool center) {
    float hp_x = center ? static_cast<float>(x + m_offset_x * scale) - (m_bar_width / 2.0f * scale) : x + m_offset_x * scale;
    float hp_y = static_cast<float>(y + m_offset_y * scale);
    float outline_x = center ? static_cast<float>(x) - (m_bar_width / 2.0f * scale) : x;

    m_bar_bg.setPosition(outline_x, static_cast<float>(y));
    m_bar_bg.setScale(scale, scale);
    g_window->draw(m_bar_bg);

    float hp_percent = 0 != max_hp ? hp / static_cast<float>(max_hp) : 0.0f;
    m_hp_bar.setPosition(hp_x, hp_y);
    m_hp_bar.setTextureRect(sf::IntRect(0, 0, static_cast<int32_t>(hp_percent * m_bar_width), 8));
    m_hp_bar.setScale(scale, scale);
    g_window->draw(m_hp_bar);

    m_text.setFont(g_font);
    m_text.setCharacterSize(8 * scale);
    m_text.setFillColor(sf::Color(255, 255, 0));
    m_text.setStyle(sf::Text::Bold);
    m_text.setString(std::format("{} / {}", hp, max_hp));
    m_text.setPosition(hp_x, hp_y);
    g_window->draw(m_text);
}

void HpBar::draw_exp_bar(int32_t x, int32_t y, int32_t exp, int32_t max_exp, float scale, bool center) {
    float exp_x = center ? static_cast<float>(x + m_offset_x * scale) - (m_bar_width / 2.0f * scale) : x + m_offset_x * scale;
    float exp_y = static_cast<float>(y + m_offset_y * scale);
    float outline_x = center ? static_cast<float>(x) - (m_bar_width / 2.0f * scale) : x;

    m_bar_bg.setPosition(outline_x, static_cast<float>(y));
    m_bar_bg.setScale(scale, scale);
    g_window->draw(m_bar_bg);

    float exp_percent = 0 != max_exp ? exp / static_cast<float>(max_exp) : 0.0f;
    m_exp_bar.setPosition(exp_x, exp_y);
    m_exp_bar.setTextureRect(sf::IntRect(0, 0, static_cast<int32_t>(exp_percent * m_bar_width), 8));
    m_exp_bar.setScale(scale, scale);
    g_window->draw(m_exp_bar);

    m_text.setFont(g_font);
    m_text.setCharacterSize(8 * scale);
    m_text.setFillColor(sf::Color(255, 255, 0));
    m_text.setStyle(sf::Text::Bold);
    m_text.setString(std::format("{} / {}", exp, max_exp));
    m_text.setPosition(exp_x, exp_y);
    g_window->draw(m_text);
}

void HpBar::draw_hp_and_exp(int32_t x, int32_t y, int32_t hp, int32_t max_hp, int32_t exp, int32_t max_exp, float scale, bool center) {
    draw_hp_bar(x, y, hp, max_hp, scale, center);
    draw_exp_bar(x, y + 17 * scale, exp, max_exp, scale, center);
}
