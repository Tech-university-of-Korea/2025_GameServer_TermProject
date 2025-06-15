#include "pch.h"
#include "network_modules.h"

void print_login_fail_reason(char reason)
{
	printf("login failure\n");
	switch (reason) {
	case LOGIN_FAIL_REASON_UNKNOWN:
		printf("Unknown Error occurred\n");
		break;

	case LOGIN_FAIL_REASON_OVER_MAX_LEN:
		printf("ID Exceeds the maximum allowed length\n");
		break;

	case LOGIN_FAIL_REASON_INVALID_ID:
		printf("Invalid ID: check your id\n");
		break;

	default:
		break;
	}
}

void process_packet(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1]) {
	case S2C_P_AVATAR_INFO:
	{
		sc_packet_avatar_info* packet = reinterpret_cast<sc_packet_avatar_info*>(ptr);
		g_myid = packet->id;
		avatar.m_id = g_myid;
		avatar.m_hp = packet->hp;
		avatar.m_max_hp = packet->max_hp;
		avatar.move(packet->x, packet->y);
		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.show();
	}
	break;

	case S2C_P_ENTER:
	{
		sc_packet_enter* my_packet = reinterpret_cast<sc_packet_enter*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
			avatar.show();
		}
		else if (id < MAX_USER) {
			players[id] = Object{ *pieces, 0, 0, 64, 64 };
			players[id].m_id = id;
			players[id].m_hp = my_packet->hp;
			players[id].m_max_hp = my_packet->max_hp;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		else {
			players[id] = Object{ *pieces, 256, 0, 64, 64 };
			players[id].m_id = id;
			players[id].m_hp = my_packet->hp;
			players[id].m_max_hp = my_packet->max_hp;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		break;
	}
	case S2C_P_MOVE:
	{
		sc_packet_move* my_packet = reinterpret_cast<sc_packet_move*>(ptr);
		int32_t other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
		}
		else {
			players[other_id].move(my_packet->x, my_packet->y);
		}
	}
	break;

	case S2C_P_LEAVE:
	{
		sc_packet_leave* my_packet = reinterpret_cast<sc_packet_leave*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			players.erase(other_id);
		}
	}
	break;

	case S2C_P_CHAT:
	{
		sc_packet_chat* my_packet = reinterpret_cast<sc_packet_chat*>(ptr);
		if (SYSTEM_ID == my_packet->id) {
			std::cout << std::format("SYSTEM: {}\n", my_packet->message);
			auto current_time = std::chrono::system_clock::now();
			g_system_messages.emplace_front(my_packet->message, current_time);
			break;
		}

		int other_id = my_packet->id;
		if (MAX_USER <= other_id) {
			players[other_id].set_chat(my_packet->message);
		}
		else {
			// TODO Chatting system
		}
	}
	break;

	case S2C_P_LOGIN_FAIL:
	{
		sc_packet_login_fail* packet = reinterpret_cast<sc_packet_login_fail*>(ptr);
		print_login_fail_reason(packet->reason);
		g_window->close();
	}
	break;

	case S2C_P_ATTACK:
	{
		sc_packet_attack* packet = reinterpret_cast<sc_packet_attack*>(ptr);
		players[packet->id].m_hp = packet->hp;
	}
	break;

	case S2C_P_LEVEL_UP:
	{
		sc_packet_level_up* packet = reinterpret_cast<sc_packet_level_up*>(ptr);
		players[packet->id].m_level = packet->level;
		players[packet->id].m_exp = packet->exp;
	}

	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
		break;
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			process_packet(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}
