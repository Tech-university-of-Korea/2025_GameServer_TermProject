#pragma once

enum COMP_TYPE : int32_t {
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
	char _send_buf[BUF_SIZE]{ };
	COMP_TYPE _comp_type;
	void* extra_info;

	OverExp() {
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		::memset(&_over, 0, sizeof(_over));
	}

	OverExp(char* packet) {
		auto size = static_cast<unsigned char>(packet[0]);
		_wsabuf.len = size;
		_wsabuf.buf = _send_buf;
		::memset(&_over, 0, sizeof(_over));
		_comp_type = OP_SEND;
		::memcpy(_send_buf, packet, size);
	}
};