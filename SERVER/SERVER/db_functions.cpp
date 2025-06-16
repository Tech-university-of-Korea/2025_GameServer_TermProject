#include "pch.h"
#include "db_functions.h"

std::pair<bool, DbUserInfo> db_login(int32_t session_id)
{
	DbUserInfo user_info{ };

	SQLRETURN retcode;
	SQLWCHAR user_id[NAME_LEN]{ };
	SQLINTEGER user_x{ };
	SQLINTEGER user_y{ };
	SQLINTEGER user_exp{ };
	SQLINTEGER user_level{ };

	SQLLEN cb_user_id = 0, cb_user_x = 0, cb_user_y = 0, cb_user_exp = 0, cb_user_level = 0;

	auto entity = g_server.get_server_object(session_id);
	if (nullptr == entity) {
		return std::make_pair(false, user_info);
	}

	std::string id{ entity->get_name() };
	auto wexec = db_make_exec("get_rpg_user_info", id);

	retcode = SQLExecDirect(h_stmt, (SQLWCHAR*)wexec.data(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

		retcode = SQLBindCol(h_stmt, 1, SQL_C_WCHAR, &user_id, NAME_LEN, &cb_user_id);
		retcode = SQLBindCol(h_stmt, 2, SQL_C_LONG, &user_x, PHONE_LEN, &cb_user_x);
		retcode = SQLBindCol(h_stmt, 3, SQL_C_LONG, &user_y, PHONE_LEN, &cb_user_y);
		retcode = SQLBindCol(h_stmt, 4, SQL_C_LONG, &user_exp, PHONE_LEN, &cb_user_exp);
		retcode = SQLBindCol(h_stmt, 5, SQL_C_LONG, &user_level, PHONE_LEN, &cb_user_level);

		retcode = SQLFetch(h_stmt);
		if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
			show_error();
			SQLCloseCursor(h_stmt);
			return std::make_pair(false, user_info);
		}

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			user_info.x = static_cast<int16_t>(user_x);
			user_info.y = static_cast<int16_t>(user_y);
			user_info.exp = static_cast<int16_t>(user_exp);
			user_info.level = static_cast<int16_t>(user_level);
			user_id[cb_user_id] = 0;
			printf("login confirm: id: [%ls],  x: %d y: %d exp: %d level: %d\n", user_id, user_x, user_y, user_exp, user_level);
			SQLCloseCursor(h_stmt);
			return std::make_pair(true, user_info);
		}
		else {
			std::cout << "not exist user: " << id << "\n";

			SQLCloseCursor(h_stmt);
			return std::make_pair(false, user_info);
		}
	}
	else {
		HandleDiagnosticRecord(h_stmt, SQL_HANDLE_STMT, retcode);
		SQLCloseCursor(h_stmt);
		return std::make_pair(false, user_info);
	}
}

bool db_create_new_user(int32_t session_id, int32_t user_x, int32_t user_y)
{
	auto entity = g_server.get_server_object(session_id);
	if (nullptr == entity) {
		return false;
	}

	std::string id{ entity->get_name() };
	std::wstring wexec = db_make_exec("create_user_info", id, user_x, user_y);
	SQLRETURN retcode;
	retcode = SQLExecDirect(h_stmt, (SQLWCHAR*)wexec.data(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
			show_error();
			SQLCloseCursor(h_stmt);
			return false;
		}

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			printf("create new user info - id: [%s], x: %d y: %d\n", id.c_str(), user_x, user_y);
			SQLCloseCursor(h_stmt);
			return true;
		}
		else {
			std::cout << "create new user failure" << "\ns";
			SQLCloseCursor(h_stmt);
			return false;
		}
	}
	else {
		HandleDiagnosticRecord(h_stmt, SQL_HANDLE_STMT, retcode);
		SQLCloseCursor(h_stmt);
		return false;
	}
}

void db_update_user_info(std::string_view name, DbUserInfo& user_info)
{
	std::string user_id{ name };
	std::wstring wexec = db_make_exec("update_user_info", user_id, user_info.x, user_info.y, user_info.level, user_info.exp);
	SQLRETURN retcode;
	retcode = SQLExecDirect(h_stmt, (SQLWCHAR*)wexec.data(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
			show_error();
			SQLCloseCursor(h_stmt);
			return;
		}

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			printf("update confirm: id: [%s], x: %d y: %d exp: %d level: %d\n", user_id.c_str(), user_info.x, user_info.y, user_info.exp, user_info.level);
			SQLCloseCursor(h_stmt);
			return;
		}
		else {
			std::cout << "update failure" << "\ns";
			SQLCloseCursor(h_stmt);
			return;
		}
	}
	else {
		HandleDiagnosticRecord(h_stmt, SQL_HANDLE_STMT, retcode);
		SQLCloseCursor(h_stmt);
		return;
	}
}