#pragma once

#include <sstream>

struct DbUserInfo {
    int16_t x;
    int16_t y;
    int32_t exp;
    int32_t level;
};

template <typename... Args>
inline std::wstring db_make_exec(std::string procedure, Args&&... params)
{
    std::ostringstream os;
    os << "EXEC " << procedure;

    bool first_param = true;
    static auto to_string_params = [&](auto&& param) {
        using Type = std::decay_t<decltype(param)>;
        if (first_param) {
            os << " ";
        }
        else {
            os << ", ";
        }
        first_param = false;

        if constexpr (std::is_convertible_v<Type, std::string>) {
            os << "\'" << param << "\'";
        }
        else {
            os << param;
        }
        };

    (to_string_params(std::forward<Args>(params)), ...);
    auto exec = os.str();

    std::wstring wexec{ };
    wexec.assign(exec.begin(), exec.end());
    return wexec;
}

std::pair<bool, DbUserInfo> db_login(int32_t session_id);
bool db_create_new_user(int32_t session_id, int32_t user_x, int32_t user_y);
void db_update_user_info(std::string_view name, DbUserInfo& user_info);
