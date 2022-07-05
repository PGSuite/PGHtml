#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include <libpq-fe.h>

#include "util-str.h"

#define PG_CLIENT_ENCODING "UTF8"
#define PG_VARCHAR_LENGTH  32767

PGconn *pg_conn = 0;
PGconn *pg_get_conn() { return pg_conn; }

void pg_connect(char *pg_uri) {
	stdout_printf('p', "\nconnecting to database... ");
    pg_conn = PQconnectdb(pg_uri);
    if (PQstatus(pg_conn) != CONNECTION_OK)
    {
    	stderr_printf(1, PQerrorMessage(pg_conn));
    	PQfinish(pg_conn);
    	pg_conn = NULL;
		strout_print_and_exit();
    }
    PQsetClientEncoding(pg_conn, PG_CLIENT_ENCODING);
    stdout_printf('p', " done, server version: %d, user: %s, client_encoding: %s", PQserverVersion(pg_conn), PQuser(pg_conn), pg_encoding_to_char(PQclientEncoding(pg_conn)));
}

void pg_disconnect() {
	if (pg_conn=NULL) return;
	PQfinish(pg_conn);
	pg_conn = NULL;
	stdout_printf('p', "\ndisconnected from database");
}

int pg_get_string(const char *query, char *value) {
	PGresult *pg_res;
	pg_res = PQexec(pg_conn, query);
    if(PQresultStatus(pg_res) != PGRES_TUPLES_OK) {
    	printf("Error on execute sql: %s\n%s", query, PQerrorMessage(pg_conn));
    	value[0]=0;
    	return 1;
    }
    char *res_value = PQgetvalue(pg_res, 0, 0);
    if (res_value==NULL)
    	res_value = "";
   	int res_len = strlen(res_value);
	if (res_len>=PG_VARCHAR_LENGTH-1) {
		printf("SQL query result too long (%d bytes)\n", res_len);
	    PQclear(pg_res);
	    return 1;
	}
	for (int i=0; i<res_len; i++)
		value[i]=res_value[i];
    value[res_len]=0;
    PQclear(pg_res);
	return 0;
}
