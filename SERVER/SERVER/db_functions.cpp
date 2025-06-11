#include "pch.h"
#include "db_functions.h"

std::pair<bool, DB_USER_INFO> db_login(int32_t session_id, const char* id)
{
	DB_USER_INFO user_info{ };

	SQLRETURN retcode;
	SQLWCHAR user_id[NAME_LEN]{ };
	SQLINTEGER user_x{ };
	SQLINTEGER user_y{ };
	SQLLEN cb_user_id = 0, cb_user_x = 0, cb_user_y = 0;

	auto session = g_server.get_session(session_id);
	if (nullptr == session) {
		return std::make_pair(false, user_info);
	}

	std::string exec{ "EXEC get_user_info " + std::string(id) };
	std::wstring wexec;
	wexec.assign(exec.begin(), exec.end());

	retcode = SQLExecDirect(h_stmt, (SQLWCHAR*)wexec.data(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

		// Bind columns 1, 2, and 3  
		retcode = SQLBindCol(h_stmt, 1, SQL_C_WCHAR, &user_id, NAME_LEN, &cb_user_id);
		retcode = SQLBindCol(h_stmt, 2, SQL_C_LONG, &user_x, PHONE_LEN, &cb_user_x);
		retcode = SQLBindCol(h_stmt, 3, SQL_C_LONG, &user_y, PHONE_LEN, &cb_user_y);

		retcode = SQLFetch(h_stmt);
		if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
			show_error();
			SQLCloseCursor(h_stmt);
			return std::make_pair(false, user_info);
		}

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			user_info.x = static_cast<int16_t>(user_x);
			user_info.y = static_cast<int16_t>(user_y);
			user_id[cb_user_id] = 0;
			printf("login confirm: id: [%ls],  x: %d y: %d\n", user_id, user_x, user_y);
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

void db_update_user_pos(int32_t session_id)
{
	auto session = g_server.get_session(session_id);
	if (nullptr == session) {
		return;
	}

	if (ST_INGAME != session->get_state()) {
		return;
	}

	std::string user_id{ session->get_name() };
	if (g_server.is_dummy_client(user_id)) {
		return;
	}

	auto [x, y] = session->get_position();

	std::string exec{ "EXEC store_user_pos '" + user_id + "', " + std::to_string(x) + ", " + std::to_string(y) };
	std::wstring wexec;
	wexec.assign(exec.begin(), exec.end());

	SQLRETURN retcode;
	retcode = SQLExecDirect(h_stmt, (SQLWCHAR*)wexec.data(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
			show_error();
			SQLCloseCursor(h_stmt);
			return;
		}

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			printf("update confirm: id: [%s], x: %d y: %d\n", user_id.c_str(), x, y);
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