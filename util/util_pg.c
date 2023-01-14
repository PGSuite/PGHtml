#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <libpq-fe.h>
// #include <server/catalog/pg_type_d.h>

#include "utils.h"

#define SQL_SESSION_ID "select " PG_SQL_SESSION_ID_COLUMN " from pg_stat_activity where pid=pg_backend_pid()"

pg_connection pg_connections[PG_CONNECTIONS_SIZE];
thread_mutex  pg_connections_mutex;

void _pg_initialize(char *error_prefix) {
	if (_thread_mutex_init_try(&pg_connections_mutex))
		_log_error_init_mutex("pg_connections_lock");
	for(int i=0; i<PG_CONNECTIONS_SIZE; i++)
		pg_connections[i].assigned = 0;
}

int pg_select_str(char *str, int str_size, PGconn *pg_conn, char *query, int params_len, ...);

int pg_connection_assign(pg_connection **pg_connection, char *uri) {
	int index;
	thread_mutex_lock(&pg_connections_mutex);
	for(index=0; index<PG_CONNECTIONS_SIZE && pg_connections[index].assigned; index++);
	if (index==PG_CONNECTIONS_SIZE) {
		thread_mutex_unlock(&pg_connections_mutex);
		return log_error(59, PG_CONNECTIONS_SIZE);
	}
	pg_connections[index].assigned = 1;
	thread_mutex_unlock(&pg_connections_mutex);
	PGconn *pg_conn;
	if (pg_connect(&pg_conn, uri)) {
		pg_connections[index].assigned = 0;
		return 1;
	}
	pg_connections[index].conn = pg_conn;
    char session_id[STR_SIZE];
	if (pg_select_str(session_id, sizeof(session_id), pg_conn, SQL_SESSION_ID, NULL) ||
		str_format(pg_connections[index].id, sizeof(pg_connections[index].id), "%03d-%s", index, session_id))
	{
		pg_connection_free(&pg_connections[index]);
		return 1;
	}
	if (thread_mutex_init(&pg_connections[index].mutex)) {
		pg_connection_free(&pg_connections[index]);
		return 1;
	}
	pg_connections[index].assigned = 2;
    log_info("pg_connections[%d] assigned, id: %s", index, pg_connections[index].id);
	*pg_connection = &pg_connections[index];
    return 0;
}

int pg_connection_id_list(stream *list) {
	if (stream_clear(list)) return 1;
	thread_mutex_lock(&pg_connections_mutex);
	int res = 0;
	for(int i=0; i<PG_CONNECTIONS_SIZE; i++) {
		if (pg_connections[i].assigned!=2) continue;
		if (stream_add_str(list, list->len>0 ? "," : "" , pg_connections[i].id, NULL)) { res = 1; break; };
	}
	thread_mutex_unlock(&pg_connections_mutex);
	return res;
}

int pg_connection_lock(pg_connection **pg_connection, char *connection_id) {
	if (connection_id[0]==0 || connection_id[1]==0 || connection_id[2]==0 || connection_id[3]!='-')
		return log_error(60, connection_id);
	int index = 0;
	for(int i=0; i<3; i++) {
		char c = connection_id[i];
		if (c<'0' || c>'9')
			return log_error(60, connection_id);
		index = index*10 + (c-'0');
	}
	if (index>=PG_CONNECTIONS_SIZE)
		return log_error(60, connection_id);
	thread_mutex_lock(&pg_connections_mutex);
	if(pg_connections[index].assigned!=2 && strcmp(pg_connections[index].id, connection_id)) {
		thread_mutex_unlock(&pg_connections_mutex);
		return log_error(61, connection_id);
	}
	thread_mutex_lock(&pg_connections[index].mutex);
	thread_mutex_unlock(&pg_connections_mutex);
	*pg_connection = &pg_connections[index];
	return 0;
}

void pg_connection_unlock(pg_connection *pg_connection) {
	thread_mutex_unlock(&pg_connection->mutex);
}

void pg_connection_free(pg_connection *pg_connection) {
	pg_disconnect(&(pg_connection->conn));
	thread_mutex_lock(&pg_connections_mutex);
	pg_connection->assigned = 3;
	thread_mutex_destroy(&pg_connection->mutex);
	pg_connection->assigned = 0;
	log_info("pg_connections[%d] freed, id: %s", (pg_connection-&pg_connections[0]), pg_connection->id);
	pg_connection->id[0] = 0;
	thread_mutex_unlock(&pg_connections_mutex);
}

int pg_uri_build(char *uri, int uri_size, char *host, char *port, char *db_name, char *user, char *password) {
	if (str_copy(uri, uri_size, "postgresql://")) return 1;
	if (host!=NULL && str_add(uri, uri_size, host, NULL)) return 1;
	if (port!=NULL && str_add(uri, uri_size, ":", port, NULL)) return 1;
	if (str_add(uri, uri_size, "/", db_name, "?connect_timeout=10", NULL)) return 1;
	if (user!=NULL      && str_add(uri, uri_size, "&user=", user, NULL)) return 1;
	if (password !=NULL && str_add(uri, uri_size, "&password=", password, NULL)) return 1;
	return 0;
}

int pg_uri_mask_password(char *uri_masked, int uri_masked_size, char *uri) {
	int uri_len = strlen(uri);
	int password_pos = str_find(uri, uri_len, 0, "password=", 1);
	if (password_pos==-1)
		return str_copy(uri_masked, uri_masked_size, uri);
	if (str_substr(uri_masked, uri_masked_size, uri, 0, password_pos+8))
		return 1;
	if (str_add(uri_masked, uri_masked_size,"*","*","*", NULL)) return 1;
	password_pos = str_find(uri, uri_len, password_pos, "&", 0);
	if (password_pos!=-1) {
		char s[STR_SIZE];
		if (str_substr(s, sizeof(s), uri, password_pos, uri_len-1)) return 1;
		if (str_add(uri_masked, uri_masked_size, s, NULL)) return 1;
	}
	return 0;
}

int pg_connect(PGconn **pg_conn, char *uri) {
	char uri_masked[STR_SIZE];
	if (pg_uri_mask_password(uri_masked, sizeof(uri_masked), uri))
		return 1;
	log_info("connecting to database, URI: %s", uri_masked);
	*pg_conn = PQconnectdb(uri);
    if (PQstatus(*pg_conn) != CONNECTION_OK)
    {
    	log_error(62, PQerrorMessage(*pg_conn));
    	PQfinish(*pg_conn);
    	return 1;
    }
    PQsetClientEncoding(*pg_conn, PG_CLIENT_ENCODING);
    log_info("connected, pid: %d, user: %s, client_encoding: %s, server version: %d", PQbackendPID(*pg_conn), PQuser(*pg_conn), pg_encoding_to_char(PQclientEncoding(*pg_conn)), PQserverVersion(*pg_conn));
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
	if (return_data==2 && PQntuples(pg_result)==0) {
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
	res = str_copy(str, str_size, PQgetvalue(pg_result, 0, 0));
    PQclear(pg_result);
    return res;
}

int pg_select_str_dflt(char *str, int str_size, char *str_dflt, PGconn *pg_conn, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	PGresult *pg_result;
	int res = _pg_execute(pg_conn, &pg_result, 1, query, params_len, args);
	va_end(args);
	if (res) return 1;
	res = str_copy(str, str_size, PQntuples(pg_result)>0 ? PQgetvalue(pg_result, 0, 0) : str_dflt);
    PQclear(pg_result);
    return res;
}


int pg_select_bool(int *value, PGconn *pg_conn, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	PGresult *pg_result;
	int res = _pg_execute(pg_conn, &pg_result, 2, query, params_len, args);
	va_end(args);
	if (res) return 1;
	*value = pg_str_to_bool(PQgetvalue(pg_result, 0, 0));
    PQclear(pg_result);
    return res;
}

int pg_select(PGresult **pg_result, PGconn *pg_conn, char *query, int params_len, ...) {
	va_list args;
	va_start(args, &params_len);
	int res = _pg_execute(pg_conn, pg_result, 1, query, params_len, args);
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

int pg_sql_params_init(pg_sql_params *sql_params, json *json, ...) {
	sql_params->len = 0;
	va_list args;
	va_start(args, &json);
    while(1)  {
    	char *json_key = va_arg(args, char *);
    	if (json_key==NULL) break;
    	json_entry *json_entry;
    	if (json_get_array_entry(&json_entry, json, 1, json_key, NULL)>0) {
    		va_end(args);
    		return 1;
    	}
    	if ((sql_params->len+json_entry->array_size)>PG_SQL_PARAMS_SIZE) {
    		va_end(args);
    		return log_error(65, sql_params->len+json_entry->array_size);
    	}
    	for(int i=0; i<json_entry->array_size; i++) {
    		if (stream_init(&sql_params->streams[sql_params->len])) {
    			va_end(args);
    			pg_sql_params_free(sql_params);
    			return 1;
    		}
    		sql_params->len++;
    		if (json_get_array_stream(&sql_params->streams[sql_params->len-1], json, json_entry, i)) {
    			va_end(args);
    			pg_sql_params_free(sql_params);
    			return 1;
    		}
    		sql_params->data[sql_params->len-1] = sql_params->streams[sql_params->len-1].data;
    	}
    }
	va_end(args);
	return 0;
}

void pg_sql_params_free(pg_sql_params *sql_params) {
	for(int i=0; i<sql_params->len; i++)
		stream_free(&sql_params->streams[i]);
}
