#pragma once

class HpBar {
public:
    static void initialize_texture();
    static void destroy_texture();

private:
    inline static sf::Texture* m_outline_tex{ nullptr };
    inline static sf::Texture* m_hp_bar_tex{ nullptr };
    inline static int32_t m_hp_width{ 100 };
    inline static int32_t m_offset{ 8 };

public:
    HpBar();
    ~HpBar();

public:
    void draw(int32_t x, int32_t y, int32_t hp, int32_t max_hp);

private:
    sf::Sprite m_hp_bar{ };
    sf::Sprite m_outline{ };
};

