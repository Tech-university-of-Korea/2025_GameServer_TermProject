#pragma once

class HpBar {
public:
    static void initialize_texture();
    static void destroy_texture();

private:
    inline static sf::Texture* m_bar_bg_tex{ nullptr };
    inline static sf::Texture* m_hp_bar_tex{ nullptr };
    inline static sf::Texture* m_exp_bar_tex{ nullptr };
    inline static int32_t m_bar_width{ 277 };
    inline static int32_t m_offset_x{ 5 };
    inline static int32_t m_offset_y{ 4 };

public:
    HpBar();
    ~HpBar();

public:
    void draw_hp_bar(int32_t x, int32_t y, int32_t hp, int32_t max_hp, float scale, bool center = true);
    void draw_exp_bar(int32_t x, int32_t y, int32_t exp, int32_t max_ex, float scale, bool center=true);
    void draw_hp_and_exp(int32_t x, int32_t y, int32_t hp, int32_t max_hp, int32_t exp, int32_t max_exp, float scale, bool center = true);

private:
    sf::Sprite m_bar_bg;
    sf::Sprite m_hp_bar;
    sf::Sprite m_exp_bar;

    sf::Text m_text;
};

