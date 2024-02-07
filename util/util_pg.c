#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <libpq-fe.h>
// #include <server/catalog/pg_type_d.h>

#include "utils.h"

// pg_connection pg_connections[PG_CONNECTIONS_SIZE];
pg_connection *pg_connections;

void _pg_initialize(char *error_prefix) {
	if (thread_mem_alloc(&pg_connections, sizeof(pg_connection)*PG_CONNECTIONS_SIZE))
		log_exit_fatal();
	for(int i=0; i<PG_CONNECTIONS_SIZE; i++) {
		if (thread_mutex_init(&pg_connections[i].mutex, "pg_connections_mutex"))
			log_exit_fatal();
		pg_connections[i].index          = i;
		pg_connections[i].assigned       = 0;
		pg_connections[i].activity_id[0] = 0;
		pg_connections[i].key[0]         = 0;
	}
}

int pg_select_str(char *str, int str_size, PGconn *pg_conn, char *query, int params_len, ...);

int pg_connection_assign(pg_connection **pg_connection, char *uri) {
	int index;
	for(index=0; index<PG_CONNECTIONS_SIZE; index++) {
		if (pg_connections[index].assigned) continue;
		if (thread_mutex_try_lock(&pg_connections[index].mutex)) continue;
		if (pg_connections[index].assigned) {
			thread_mutex_unlock(&pg_connections[index].mutex);
			continue;
		} else {
			pg_connections[index].assigned = 1;
			thread_mutex_unlock(&pg_connections[index].mutex);
			break;
		}
	}
	if (index==PG_CONNECTIONS_SIZE) return log_error(59, PG_CONNECTIONS_SIZE);
	PGconn *pg_conn;
	if (pg_connect(&pg_conn, uri)) {
		pg_connections[index].assigned = 0;
		return 1;
	}
	pg_connections[index].conn = pg_conn;
	if (pg_select_str(pg_connections[index].activity_id, sizeof(pg_connections[index].activity_id), pg_conn, PG_SQL_ACTIVITY_ID " where pid=pg_backend_pid()", NULL)) {
		pg_connection_free(&pg_connections[index]);
		return 1;
	}
	int t = time(0);
	srand(t+index);
	for(int i=0; i<PG_CONNECTION_KEY_SIZE-1; i++) {
		int r = (i<6 ? t/(i+1) : i==6 ? strlen(uri) : i==7 ? index: rand())%36;
		pg_connections[index].key[i] = (i%9==8) ? '-' : r<10 ? '0'+r : 'A'+(r-10);
	}
	pg_connections[index].key[PG_CONNECTION_KEY_SIZE-1] = 0;
	pg_connections[index].assigned = 2;
    log_info("pg_connections[%d] assigned, activity_id: %s", index, pg_connections[index].activity_id);
	*pg_connection = &pg_connections[index];
    return 0;
}

int pg_connection_array_sql(stream *array) {
	stream_clear(array);
	if (stream_add_char(array, '{')) return 1;
	for(int i=0; i<PG_CONNECTIONS_SIZE; i++) {
		if (pg_connections[i].assigned!=2) continue;
		char array_item[sizeof(pg_connections[i].activity_id)+20];
		if (str_format(array_item, sizeof(array_item), "\"{%d,%s}\"", pg_connections[i].index, pg_connections[i].activity_id)) return 1;
		if (stream_add_str(array, array->len>1 ? "," : "", array_item, NULL))  return 1;
	}
	if (stream_add_char(array, '}')) return 1;
	return 0;
}

int pg_connection_lock(pg_connection **pg_connection, int index) {
	if (index<0 || index>=PG_CONNECTIONS_SIZE)
		return log_error(60, index);
	thread_mutex_lock(&pg_connections[index].mutex);
	if (pg_connections[index].assigned!=2) return log_error(71);
	*pg_connection = &pg_connections[index];
	return 0;
}

void pg_connection_unlock(pg_connection *pg_connection) {
	thread_mutex_unlock(&pg_connection->mutex);
}

int pg_connection_check_key(int index, char *key) {
	if (strcmp(pg_connections[index].key,key)) return log_error(65);
	return 0;
}

void pg_connection_free(pg_connection *pg_connection) {
	pg_connection->assigned       = 3;
	pg_connection->activity_id[0] = 0;
	pg_connection->key[0]         = 0;
	pg_disconnect(&(pg_connection->conn));
	pg_connection->assigned = 0;
	log_info("pg_connections[%d] freed, activity_id: %s", (pg_connection-&pg_connections[0]), pg_connection->activity_id);
}

void pg_connection_try_free(int index, char *activity_id) {
	if (thread_mutex_try_lock(&pg_connections[index].mutex)) return;
	if (pg_connections[index].assigned==2)
		pg_connection_free(&pg_connections[index]);
	thread_mutex_unlock(&pg_connections[index].mutex);
}

int pg_uri_build(char *uri, int uri_size, char *host, char *port, char *db_name, char *user, char *password) {
	if (str_copy(uri, uri_size, "postgresql://")) return 1;
	if (host!=NULL && str_add(uri, uri_size, host, NULL)) return 1;
	if (port!=NULL && str_add(uri, uri_size, ":", port, NULL)) return 1;
	if (str_add(uri, uri_size, "/", db_name, "?connect_timeout=10", NULL)) return 1;
	if (user!=NULL     && str_add(uri, uri_size, "&user=", user, NULL)) return 1;
	if (password!=NULL && str_add(uri, uri_size, "&password=", password, NULL)) return 1;
	return 0;
}

int pg_uri_mask_password(char *uri_masked, int uri_masked_size, char *uri) {
	int password_pos = str_find(uri, 0, "password=", 1);
	if (password_pos==-1)
		return str_copy(uri_masked, uri_masked_size, uri);
	if (str_substr(uri_masked, uri_masked_size, uri, 0, password_pos+8))
		return 1;
	if (str_add(uri_masked, uri_masked_size,"*","*","*", NULL)) return 1;
	password_pos = str_find(uri, password_pos, "&", 0);
	if (password_pos!=-1) {
		char s[STR_SIZE];
		if (str_substr(s, sizeof(s), uri, password_pos, strlen(uri)-1)) return 1;
		if (str_add(uri_masked, uri_masked_size, s, NULL)) return 1;
	}
	return 0;
}

void _PQnoticeProcessorDisable(void *arg, const char *message) {
    // log_info("SQL %s", message);
}

int pg_connect(PGconn **pg_conn, char *uri) {
	char uri_masked[STR_SIZE];
	if (pg_uri_mask_password(uri_masked, sizeof(uri_masked), uri))
		return 1;
	log_info("connecting to database, URI: %s", uri_masked);
	*pg_conn = PQconnectdb(uri);
    if (PQstatus(*pg_conn) != CONNECTION_OK)  {
    	log_error(62, PQerrorMessage(*pg_conn));
    	PQfinish(*pg_conn);
    	*pg_conn = NULL;
    	return 1;
    }
    PQsetClientEncoding(*pg_conn, PG_CLIENT_ENCODING);
    PQsetNoticeProcessor(*pg_conn, _PQnoticeProcessorDisable, NULL);
    log_info("connected to database, pid: %d, user: %s, client_encoding: %s, server version: %d", PQbackendPID(*pg_conn), PQuser(*pg_conn), pg_encoding_to_char(PQclientEncoding(*pg_conn)), PQserverVersion(*pg_conn));
    return 0;
}

void pg_disconnect(PGconn **pg_conn) {
	if (pg_conn==NULL) return;
	int pid = PQbackendPID(*pg_conn);
	PQfinish(*pg_conn);
	*pg_conn = NULL;
	log_info("disconnected from database, pid: %d", pid);
}

int pg_str_to_bool(char *value) {
	if (value==NULL) return 0;
    return value[0]=='t';
}

// return_data:
//   !0 - return data (select)
//    2 - not empty
//    3 - one row
int pg_check_result(PGresult *pg_result, const char *query, int return_data) {
	int pg_result_status = PQresultStatus(pg_result);
	if (pg_result_status==PGRES_FATAL_ERROR || pg_result_status==PGRES_BAD_RESPONSE) {
		log_error(19, PQresultErrorField(pg_result, PG_DIAG_SQLSTATE), query, PQresultErrorMessage(pg_result));
		PQclear(pg_result);
		return 1;
	}
    if (return_data && pg_result_status!=PGRES_TUPLES_OK) {
    	PQclear(pg_result);
    	return log_error(40, query);
    }
	if ( (return_data==2 || return_data==3) && PQntuples(pg_result)==0) {
		PQclear(pg_result);
		return log_error(66, query);
	}
	if (return_data==3 && PQntuples(pg_result)>1) {
		PQclear(pg_result);
		return log_error(77, query);
	}
	return 0;
}

int _pg_execute(PGconn *pg_conn, PGresult **pg_result, int return_data, char *query, int params_len, va_list args) {
	if (params_len>=PG_SQL_PARAMS_SIZE)
		return log_error(65, params_len);
	char *params[PG_SQL_PARAMS_SIZE];
	for(int i=0; i<params_len; i++)
		params[i] = va_arg(args, char *);
	*pg_result = PQexecParams(pg_conn, query, params_len, NULL, params, NULL, NULL, 0);
	return pg_check_result(*pg_result, query, return_data);
}

int pg_select_str(char *str, int str_size, PGconn *pg_conn, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	PGresult *pg_result;
	int res = _pg_execute(pg_conn, &pg_result, 2, query, params_len, args);
	va_end(args);
	if (res) return 1;
	res = str_copy(str, str_size, !PQgetisnull(pg_result, 0, 0) ? PQgetvalue(pg_result, 0, 0) : "");
    PQclear(pg_result);
    return 0;
}

int pg_select_str_dflt(char *str, int str_size, char *str_dflt, PGconn *pg_conn, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	PGresult *pg_result;
	int res = _pg_execute(pg_conn, &pg_result, 1, query, params_len, args);
	va_end(args);
	if (res) return 1;
	res = str_copy(str, str_size, PQntuples(pg_result)>0 && !PQgetisnull(pg_result, 0, 0) ? PQgetvalue(pg_result, 0, 0) : str_dflt);
    PQclear(pg_result);
    return 0;
}

int pg_select_bool(int *value, PGconn *pg_conn, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	PGresult *pg_result;
	int res = _pg_execute(pg_conn, &pg_result, 2, query, params_len, args);
	va_end(args);
	if (res) return 1;
	*value = !PQgetisnull(pg_result, 0, 0) ? pg_str_to_bool(PQgetvalue(pg_result, 0, 0)) : 0;
    PQclear(pg_result);
    return 0;
}

int pg_select(PGresult **pg_result, PGconn *pg_conn, int return_data, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	int res = _pg_execute(pg_conn, pg_result, return_data, query, params_len, args);
	va_end(args);
    return res;
}

int pg_execute(PGconn *pg_conn, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	PGresult *pg_result;
	int res = _pg_execute(pg_conn, &pg_result, 0, query, params_len, args);
	va_end(args);
	if (res) return 1;
    PQclear(pg_result);
    return res;
}
