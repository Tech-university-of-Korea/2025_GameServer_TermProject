#pragma once

class Object {
public:
	Object();
	Object(sf::Texture& t, int32_t x, int32_t y, int32_t x2, int32_t y2);

public:
	void set_name(const char str[]);
	void set_chat(const char str[]);

	void show();
	void hide();

	void move_avatar(int32_t x, int32_t y);
	void draw_avatar();

	void move(int32_t x, int32_t y);
	void draw();

public:
	int32_t m_id{ };
	int32_t m_x{ };
	int32_t m_y{ };
	int32_t m_hp{ };
	int32_t m_max_hp{ };
	int32_t m_level{ };
	char name[MAX_ID_LENGTH]{ };

private:
	bool m_showing{ false };
	sf::Sprite m_sprite{ };

	sf::Text m_name{ };
	sf::Text m_chat{ };
	std::chrono::system_clock::time_point m_mess_end_time{ };
};