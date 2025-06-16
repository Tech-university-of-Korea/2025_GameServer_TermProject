#include "pch.h"
#include "object.h"

Object::Object() {
	m_showing = false;
}

Object::Object(sf::Texture& t, int32_t x, int32_t y, int32_t x2, int32_t y2) {
	m_showing = false;
	m_sprite.setTexture(t);
	m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
	set_name("NONAME");
	m_mess_end_time = std::chrono::system_clock::now();
}

void Object::show() {
	m_showing = true;
}

void Object::hide() {
	m_showing = false;
}

void Object::move_avatar(int32_t x, int32_t y) {
	m_sprite.setPosition((float)x, (float)y);
}

void Object::draw_avatar() {
	g_window->draw(m_sprite);
}

void Object::move(int32_t x, int32_t y) {
	m_x = x;
	m_y = y;
}

void Object::draw() {
	if (false == m_showing) {
		return;
	}

	float rx = (m_x - g_left_x) * 65.0f + 1;
	float ry = (m_y - g_top_y) * 65.0f + 1;
	m_sprite.setPosition(rx, ry);

	g_window->draw(m_sprite);

	if (g_myid == m_id) {
		m_level_text.setFont(g_font);
		m_level_text.setFillColor(sf::Color(255, 255, 0));
		m_level_text.setStyle(sf::Text::Bold);
		m_level_text.setString(std::format("Level. {}", m_level));
		m_level_text.setPosition(40.0f, 50.0f);

		g_window->draw(m_level_text);
		g_hp_bar->draw_hp_and_exp(20, 90.0f, m_hp, m_max_hp, m_exp, m_max_exp, 1.5f, false);
	}
	else {
		g_hp_bar->draw_hp_bar(rx + 32, ry - 10, m_hp, m_max_hp, 0.5f);
	}

	if (MAX_USER <= m_id) {
		if (m_mess_end_time < std::chrono::system_clock::now()) {
			auto size = m_name.getGlobalBounds();
			auto y = g_myid == m_id ? ry - 10.0f : ry - 50.0f;
			m_name.setPosition(rx + 32.0f - size.width / 2.0f, y);
			g_window->draw(m_name);
		}
		else {
			auto size = m_chat.getGlobalBounds();
			auto y = g_myid == m_id ? ry - 10.0f : ry - 50.0f;
			m_chat.setPosition(rx + 32.0f - size.width / 2.0f, y);
			g_window->draw(m_chat);
		}
	}
	else {
		auto size = m_name.getGlobalBounds();
		auto y = g_myid == m_id ? ry - 10.0f : ry - 50.0f;
		m_name.setPosition(rx + 32.0f - size.width / 2.0f, y);
		g_window->draw(m_name);
	}
}

void Object::set_name(const char str[]) {
	m_name.setFont(g_font);
	m_name.setString(str);
	if (m_id < MAX_USER) {
		m_name.setFillColor(sf::Color::White);
	}
	else {
		m_name.setFillColor(sf::Color::Yellow);
	}
	m_name.setStyle(sf::Text::Bold);
}

void Object::set_chat(const char str[]) {
	m_chat.setFont(g_font);
	m_chat.setString(str);
	m_chat.setFillColor(sf::Color(255, 255, 255));
	m_chat.setStyle(sf::Text::Bold);
	m_mess_end_time = std::chrono::system_clock::now() + std::chrono::seconds(3);
}
