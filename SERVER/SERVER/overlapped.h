#pragma once

enum IoType : int32_t {
	// OP_IO
	OP_ACCEPT = 0x00, 
	OP_RECV, 
	OP_SEND,
	// OP_NPC
	OP_NPC_MOVE = 0x10, 
	OP_NPC_RESPAWN,
	OP_AI_HELLO,
	// OP_GAME_EVENT
	OP_GAME_EVENT = 0x0100
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