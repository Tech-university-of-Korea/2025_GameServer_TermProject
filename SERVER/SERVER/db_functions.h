#pragma once

struct DB_USER_INFO {
    int16_t x;
    int16_t y;
};

std::pair<bool, DB_USER_INFO> db_login(int32_t session_id, const char* id);
void db_update_user_pos(int32_t session_id);