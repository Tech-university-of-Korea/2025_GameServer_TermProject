#pragma once

class HpBar {
public:
    static void initialize_texture();
    static void destroy_texture();

private:
    inline static sf::Texture* m_outline_tex{ nullptr };
    inline static sf::Texture* m_hp_bar_tex_enemy{ nullptr };
    inline static sf::Texture* m_hp_bar_tex_player{ nullptr };
    inline static int32_t m_hp_width{ 100 };
    inline static int32_t m_offset_x{ 10 };
    inline static int32_t m_offset_y{ 1 };

public:
    HpBar();
    ~HpBar();

public:
    void draw_player_hp(int32_t x, int32_t y, int32_t hp, int32_t max_hp, float scale, bool center = true);
    void draw_enemy_hp(int32_t x, int32_t y, int32_t hp, int32_t max_hp, float scale, bool center=true);

private:
    sf::Sprite m_hp_bar_enemy{ };
    sf::Sprite m_hp_bar_player{ };
    sf::Sprite m_outline{ };
};

