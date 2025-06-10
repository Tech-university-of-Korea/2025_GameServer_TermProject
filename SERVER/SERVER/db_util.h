#pragma once

#include <sqlext.h>  
#include <locale.h>

#define NAME_LEN 50  
#define PHONE_LEN 60

inline SQLHENV h_env;
inline SQLHDBC h_dbc;
inline SQLHSTMT h_stmt = 0;

inline void show_error() {
    printf("error\n");
}

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
void init_db();
void cleanup_db();