#pragma once

constexpr short GAME_PORT = 3000;

constexpr short MAX_CHAT_LENGTH = 200;
constexpr int BUF_SIZE = 300;

constexpr int SYSTEM_ID = -1;

constexpr int  MAX_USER = 10000;
constexpr int  NUM_MONSTER = 200000;

constexpr char S2C_P_AVATAR_INFO = 1;
constexpr char S2C_P_MOVE = 2;
constexpr char S2C_P_ENTER = 3;
constexpr char S2C_P_LEAVE = 4;
constexpr char S2C_P_CHAT = 7;
constexpr char S2C_P_STAT_CHANGE = 8;
constexpr char S2C_P_LOGIN_FAIL = 9;

constexpr char C2S_P_LOGIN = 65;
constexpr char C2S_P_MOVE = 66;
constexpr char C2S_P_ATTACK = 67;
constexpr char C2S_P_CHAT = 68;
constexpr char C2S_P_TELEPORT = 69;		// 동접 테스트 할 때
										// 시작마을의 HOTSPOT을 방지하기 위해 
										// RANDOM TELEPORT할 때 사용

constexpr char MAX_ID_LENGTH = 20;

constexpr char MOVE_UP = 1;
constexpr char MOVE_DOWN = 2;
constexpr char MOVE_LEFT = 3;
constexpr char MOVE_RIGHT = 4;

constexpr unsigned short MAP_HEIGHT = 2000;
constexpr unsigned short MAP_WIDTH = 2000;

#pragma pack (push, 1)

struct sc_packet_avatar_info {
	unsigned char size;
	char type;
	long long  id;
	short x, y;
	short level;
	int   exp;
	int   max_exp;
	int hp;
	int max_hp;
};

struct sc_packet_move {
	unsigned char size;
	char type;
	long long id;
	short x;
	short y;

	long long move_time;
};

struct sc_packet_enter {
	unsigned char size;
	char type;
	long long  id;
	char name[MAX_ID_LENGTH];
	char o_type;						// 0 : PLAYER
										// 1...  : NPC들 
	short x, y;
	int hp;
	int max_hp;
};

struct sc_packet_leave {
	unsigned char size;
	char type;
	long long  id;
};

struct sc_packet_chat {
	unsigned char size;
	char type;
	long long  id;						// 메세지를 보낸 Object의 ID
										// -1 => SYSTEM MESSAGE
										//       전투 메세지 보낼때 사용
	char message[MAX_CHAT_LENGTH];		// NULL terminated 문자열
};

struct sc_packet_stat_change {
	unsigned char size;
	char type;
	long long  id;
	int hp;
	int max_hp;
	short level;
	int   exp;
	int max_exp;
};

struct sc_packet_login_fail {
	unsigned char size;
	char type;
	long long  id;
	char reason;			// 0 : 알수 없는 이유
							// 1 : 다른 클라이언트에서 사용중
							// 2 : 부적절한 ID (특수문자, 20자 이상)
							// 3 : 서버에 동접이 너무 많음
};

struct cs_packet_login {
	unsigned char  size;
	char  type;
	char  name[MAX_ID_LENGTH];
};

struct cs_packet_move {
	unsigned char  size;
	char  type;
	char  direction;

	long long move_time;
};

struct cs_packet_attack {
	unsigned char  size;
	char  type;
};

struct cs_packet_chat {
	unsigned char  size;
	char  type;
	char  message[MAX_CHAT_LENGTH];		// NULL terminated 문자열
};

struct cs_packet_teleport {
	unsigned char  size;
	char  type;
};

#pragma pack (pop)

