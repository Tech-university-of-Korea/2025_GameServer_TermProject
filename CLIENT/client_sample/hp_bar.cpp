#include "pch.h"
#include "hp_bar.h"

void HpBar::initialize_texture() {
    m_outline_tex = new sf::Texture{ };
    m_hp_bar_tex = new sf::Texture{ };

    m_outline_tex->loadFromFile("assets/hp_outline.png");
    m_hp_bar_tex->loadFromFile("assets/hp_bar.png");
}

void HpBar::destroy_texture() {
    if (nullptr != m_outline_tex) {
        delete m_outline_tex;
        m_outline_tex = nullptr;
    }

    if (nullptr != m_hp_bar_tex) {
        delete m_hp_bar_tex;
        m_hp_bar_tex = nullptr;
    }
}

HpBar::HpBar() {
    m_hp_bar.setTexture(*m_hp_bar_tex);
    m_hp_bar.setTextureRect(sf::IntRect(0, 0, 100, 8));
    m_outline.setTexture(*m_outline_tex);
    m_outline.setTextureRect(sf::IntRect(0, 0, 108, 10));
}

HpBar::~HpBar() { }

void HpBar::draw(int32_t x, int32_t y, int32_t hp, int32_t max_hp) {
    if (0 == max_hp) {
        return;
    }

    float percent = std::clamp(hp / static_cast<float>(max_hp), 0.0f, 1.0f);
    m_hp_bar.setPosition(static_cast<float>(x + m_offset) - 25, static_cast<float>(y));
    m_outline.setPosition(static_cast<float>(x) - 25, static_cast<float>(y));

    m_hp_bar.setTextureRect(sf::IntRect(0, 0, static_cast<int32_t>(percent * m_hp_width), 8));

    g_window->draw(m_outline);
    g_window->draw(m_hp_bar);
}
