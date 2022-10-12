#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include <libpq-fe.h>

#include "util_str.h"

#define PG_CLIENT_ENCODING "UTF8"

int pg_uri_build(char *uri, int uri_size, char *host, char *port, char *db_name, char *user, char *password) {

	if (str_add(uri, uri_size, "postgresql://", NULL)) return 1;
	if (host!=NULL && str_add(uri, uri_size, host, NULL)) return 1;
	if (port!=NULL && str_add(uri, uri_size, ":", port, NULL)) return 1;
	if (str_add(uri, uri_size, "/", db_name, "?connect_timeout=10", NULL)) return 1;
	if (user!=NULL      && str_add(uri, uri_size, "&user=", user, NULL)) return 1;
	if (password !=NULL && str_add(uri, uri_size, "&password=", password, NULL)) return 1;

	return 0;

}

int pg_uri_mask_password(char *uri_out, int uri_out_size, char *uri) {
	int uri_len = strlen(uri);
	int password_pos = str_find(uri, uri_len, 0, "password=", 1);
	if (password_pos==-1)
		return str_copy(uri_out, uri_out_size, uri);
	if (str_substr(uri_out, uri_out_size, uri, 0, password_pos+8))
		return 1;
	if (str_add(uri_out, uri_out_size, 3, "*","*","*", NULL)) return 1;
	password_pos = str_find(uri, uri_len, password_pos, "&", 0);
	if (password_pos!=-1) {
		char s[STR_SIZE_MAX];
		if (str_substr(s, sizeof(s), uri, password_pos, uri_len-1)) return 1;
		if (str_add(uri_out, uri_out_size, s, NULL)) return 1;
	}
	return 0;
}

int pg_connect(PGconn **pg_conn, char *uri) {
	char uri_out[STR_SIZE_MAX];
	if (pg_uri_mask_password(uri_out, sizeof(uri_out), uri))
		log_stdout_print_and_exit(2);
	log_stdout_println("connecting to database, URI: %s", uri_out);
	*pg_conn = PQconnectdb(uri);
    if (PQstatus(*pg_conn) != CONNECTION_OK)
    {
    	log_stderr_print(1, PQerrorMessage(*pg_conn));
    	PQfinish(*pg_conn);
    	return 1;
    }
    PQsetClientEncoding(*pg_conn, PG_CLIENT_ENCODING);
    log_stdout_println("connected, server version: %d, user: %s, client_encoding: %s", PQserverVersion(*pg_conn), PQuser(*pg_conn), pg_encoding_to_char(PQclientEncoding(*pg_conn)));
    return 0;
}

void pg_disconnect(PGconn *pg_conn) {
	if (pg_conn==NULL) return;
	PQfinish(pg_conn);
	log_stdout_println("disconnected from database");
}

int pg_select_str_list(PGconn *pg_conn, STR_LIST *values, char *query) {
	values->size=0;
	PGresult *pg_result = PQexec(pg_conn, query);
    if(PQresultStatus(pg_result) != PGRES_TUPLES_OK) {
    	log_stderr_print(19, query, PQerrorMessage(pg_conn));
    	return 1;
    }
	int row_count = PQntuples(pg_result);
    for(int i=0; i<row_count; i++) {
		if (str_list_add(values, PQgetvalue(pg_result, i, 0)))
			return 1;
    }
    PQclear(pg_result);
    return 0;
}

int pg_select_boolean(PGconn *pg_conn, int *value, char *query) {
	PGresult *pg_result = PQexec(pg_conn, query);
    if(PQresultStatus(pg_result) != PGRES_TUPLES_OK) {
    	log_stderr_print(19, query, PQerrorMessage(pg_conn));
    	return 1;
    }
    if (PQntuples(pg_result)==0) {
    	log_stderr_print(40, query);
    	return 1;
    }
    *value = (PQgetvalue(pg_result, 0, 0)[0]=='t');
    PQclear(pg_result);
    return 0;
}

int pg_select(PGconn *pg_conn, PGresult **result, char *query) {
	*result = PQexec(pg_conn, query);
	log_stdout_println("*** pg_select %d", PQntuples(*result));
    if(PQresultStatus(*result) != PGRES_TUPLES_OK) {
    	log_stderr_print(19, query, PQerrorMessage(pg_conn));
    	return 1;
    }
	return 0;
}
