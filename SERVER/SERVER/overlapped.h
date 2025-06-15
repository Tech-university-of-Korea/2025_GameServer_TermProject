#pragma once

enum COMP_TYPE { 
	OP_ACCEPT, 
	OP_RECV, 
	OP_SEND, 
	OP_NPC_MOVE, 
	OP_NPC_RESPAWN,
	OP_AI_HELLO 
};

class OverExp {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE]{ };
	COMP_TYPE _comp_type;
	int _ai_target_obj;

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