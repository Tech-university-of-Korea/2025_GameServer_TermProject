#include "pch.h"
#include "hp_bar.h"

void HpBar::initialize_texture() {
    m_outline_tex = new sf::Texture{ };
    m_hp_bar_tex_enemy = new sf::Texture{ };
    m_hp_bar_tex_player = new sf::Texture{ };

    m_outline_tex->loadFromFile("assets/hp_outline.png");
    m_hp_bar_tex_enemy->loadFromFile("assets/hp_bar_enemy.png");
    m_hp_bar_tex_player->loadFromFile("assets/hp_bar_player.png");;
}

void HpBar::destroy_texture() {
    if (nullptr != m_outline_tex) {
        delete m_outline_tex;
        m_outline_tex = nullptr;
    }

    if (nullptr != m_hp_bar_tex_enemy) {
        delete m_hp_bar_tex_enemy;
        m_hp_bar_tex_enemy = nullptr;
    }
}

HpBar::HpBar() {
    m_hp_bar_player.setTexture(*m_hp_bar_tex_player);
    m_hp_bar_player.setTextureRect(sf::IntRect(0, 0, 100, 8));
    m_hp_bar_enemy.setTexture(*m_hp_bar_tex_enemy);
    m_hp_bar_enemy.setTextureRect(sf::IntRect(0, 0, 100, 8));
    m_outline.setTexture(*m_outline_tex);
    m_outline.setTextureRect(sf::IntRect(0, 0, 110, 10));
}

HpBar::~HpBar() { }

void HpBar::draw_enemy_hp(int32_t x, int32_t y, int32_t hp, int32_t max_hp, float scale, bool center) {
    if (0 == max_hp) {
        return;
    }

    float hp_x = center ? static_cast<float>(x + m_offset_x * scale) - (50.0f * scale) : x + m_offset_x * scale;
    float hp_y = static_cast<float>(y + m_offset_y * scale);
    float outline_x = center ? static_cast<float>(x) - (50.0f * scale) : x;

    float percent = std::clamp(hp / static_cast<float>(max_hp), 0.0f, 1.0f);
    m_hp_bar_enemy.setPosition(hp_x, hp_y);
    m_hp_bar_enemy.setTextureRect(sf::IntRect(0, 0, static_cast<int32_t>(percent * m_hp_width), 8));
    m_hp_bar_enemy.setScale(scale, scale);
    g_window->draw(m_hp_bar_enemy);

    m_outline.setPosition(outline_x, static_cast<float>(y));
    m_outline.setScale(scale, scale);
    g_window->draw(m_outline);
}

void HpBar::draw_player_hp(int32_t x, int32_t y, int32_t hp, int32_t max_hp, float scale, bool center) {
    if (0 == max_hp) {
        return;
    }

    float hp_x = center ? static_cast<float>(x + m_offset_x * scale) - (50.0f * scale) : x + m_offset_x * scale;
    float hp_y = static_cast<float>(y + m_offset_y * scale);
    float outline_x = center ? static_cast<float>(x) - (50.0f * scale) : x;

    float percent = std::clamp(hp / static_cast<float>(max_hp), 0.0f, 1.0f);
    m_hp_bar_player.setPosition(hp_x, hp_y);
    m_hp_bar_player.setTextureRect(sf::IntRect(0, 0, static_cast<int32_t>(percent * m_hp_width), 8));
    m_hp_bar_player.setScale(scale, scale);
    g_window->draw(m_hp_bar_player);

    m_outline.setPosition(outline_x, static_cast<float>(y));
    m_outline.setScale(scale, scale);
    g_window->draw(m_outline);
}
