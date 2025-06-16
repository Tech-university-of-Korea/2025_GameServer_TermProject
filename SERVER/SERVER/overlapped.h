#pragma once

constexpr int32_t IOTYPE_IO = 0x10;
constexpr int32_t IOTYPE_TIMER = 0x100;
constexpr int32_t IOTYPE_DB = 0x1000;

enum IoType : int32_t {
	// OP_IO
	OP_ACCEPT = IOTYPE_IO,
	OP_RECV, 
	OP_SEND,
	// OP_TIMER
	OP_NPC_MOVE = IOTYPE_TIMER,
	OP_NPC_RESPAWN,
	OP_NPC_CHECK_AGRO,
	OP_GAME_EVENT,
	// OP_DB
	OP_DB_LOGIN = IOTYPE_DB,
	OP_DB_UPDATE_USER_INFO, 
	OP_DB_CREATE_USER_INFO,
};

struct OverExp {
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	IoType _comp_type;
	void* extra_info;
};

struct IoOver : public OverExp {
	char _send_buf[BUF_SIZE]{ };

	IoOver() {
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		::memset(&_over, 0, sizeof(_over));
	}

	IoOver(char* packet) {
		auto size = static_cast<unsigned char>(packet[0]);
		_wsabuf.len = size;
		_wsabuf.buf = _send_buf;
		::memset(&_over, 0, sizeof(_over));
		_comp_type = OP_SEND;
		::memcpy(_send_buf, packet, size);
	}
};