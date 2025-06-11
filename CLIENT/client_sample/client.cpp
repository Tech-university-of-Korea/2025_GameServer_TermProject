#include "pch.h"
#include "Object.h"

sf::TcpSocket s_socket;

void initialize_system_message()
{
	g_system_message.setFont(g_font);
	g_system_message.setFillColor(sf::Color::Red);
	g_system_message.setOutlineColor(sf::Color::Yellow);
	g_system_message.setCharacterSize(35);
	g_system_message.setOutlineThickness(1.0f);
	g_system_message.setStyle(sf::Text::Bold);
}

void client_initialize()
{
	board = new sf::Texture;
	pieces = new sf::Texture;
	board->loadFromFile("assets/chessmap.bmp");
	pieces->loadFromFile("assets/chess2.png");
	if (false == g_font.loadFromFile("assets/cour.ttf")) {
		std::cout << "Font Loading Error!\n";
		exit(-1);
	}
	white_tile = Object{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
	black_tile = Object{ *board, 69, 5, TILE_WIDTH, TILE_WIDTH };
	avatar = Object{ *pieces, 128, 0, 64, 64 };
	avatar.move(4, 4);

	initialize_system_message();
	HpBar::initialize_texture();

	g_hp_bar = std::make_unique<HpBar>();
}

void client_finish()
{
	players.clear();
	delete board;
	delete pieces;
	HpBar::destroy_texture();
}

void set_system_message()
{
	auto current_time = std::chrono::system_clock::now();
	std::erase_if(g_system_messages, [current_time](const auto& mess) {
		if (MESSAGE_TIMEOUT < std::chrono::duration_cast<std::chrono::milliseconds>(current_time - mess.time)) {
			return true;
		}
		return false;
		}
	);

	// 시스템 메시지 수 제한
	if (g_system_messages.size() > MAX_CNT_STORED_SYSTEM_MESS) {
		auto delete_cnt = g_system_messages.size() - MAX_CNT_STORED_SYSTEM_MESS;
		for (auto _ = 0; _ < delete_cnt; ++_) {
			g_system_messages.pop_back();
		}
	}

	std::string merged_string;
	for (const auto& mess : g_system_messages) {
		merged_string.append(mess.message + "\n");
	}

	g_system_message.setString(merged_string);
	sf::View view;
	g_window->getViewport(view);
	auto center = view.getCenter();
	center.y = 20.0f;
	g_system_message.setPosition(center);
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		std::wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		std::wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i) {
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;
			if (0 == (tile_x / 3 + tile_y / 3) % 2) {
				white_tile.move_avatar(TILE_WIDTH * i, TILE_WIDTH * j);
				white_tile.draw_avatar();
			}
			else
			{
				black_tile.move_avatar(TILE_WIDTH * i, TILE_WIDTH * j);
				black_tile.draw_avatar();
			}
		}
	}
	avatar.draw();
	for (auto& pl : players) pl.second.draw();

	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	g_window->draw(text);

	set_system_message();
	g_window->draw(g_system_message);
}

void send_packet(void *packet)
{
	unsigned char *p = reinterpret_cast<unsigned char *>(packet);
	size_t sent = 0;
	s_socket.send(packet, p[0], sent);
}

int main()
{
	std::wcout.imbue(std::locale("korean"));
	std::wcout << L"Server IP: ";
	std::wstring ip{ };
	std::wcin >> ip;
	std::wcout << L"login id 입력: ";
	std::wstring id{ };
	std::wcin >> id;

	std::string str_ip;
	str_ip.assign(ip.begin(), ip.end());
	sf::Socket::Status status = s_socket.connect(str_ip, GAME_PORT);
	s_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		std::wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}

	client_initialize();
	cs_packet_login p;
	p.size = sizeof(p);
	p.type = C2S_P_LOGIN;

	std::string player_name{ };
	player_name.assign(id.begin(), id.end());
	
	::memset(p.name, 0, MAX_ID_LENGTH);
	strcpy_s(p.name, player_name.c_str());
	send_packet(&p);
	avatar.set_name(p.name);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				int direction = -1;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = 2;
					break;
				case sf::Keyboard::Right:
					direction = 3;
					break;
				case sf::Keyboard::Up:
					direction = 0;
					break;
				case sf::Keyboard::Down:
					direction = 1;
					break;
				case sf::Keyboard::LControl:
				{
					cs_packet_attack p;
					p.size = sizeof(p);
					p.type = C2S_P_ATTACK;
					send_packet(&p);
				}
				break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				}

				if (-1 != direction) {
					cs_packet_move p;
					p.size = sizeof(p);
					p.type = C2S_P_MOVE;
					p.direction = direction;
					send_packet(&p);
				}

			}
		}

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}