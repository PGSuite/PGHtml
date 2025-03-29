#ifndef UTIL_EXT_H_
#define UTIL_EXT_H_

#include "../util/util.h"

// pg --------------------------------------------------------------------------------------------

#include <libpq-fe.h>

#define PG_CLIENT_ENCODING "UTF8"

#define PG_CONNECTIONS_SIZE     1000
#define PG_CONNECTION_KEY_SIZE  (8*(8+1))
#define PG_SQL_PARAMS_SIZE      256

#define PG_SQL_ACTIVITY_ID  "select pid||'-'||to_char(backend_start, 'yyyymmdd-hh24miss-us') activity_id from pg_stat_activity"

typedef struct
{
	int index;
	unsigned char assigned;
	thread_mutex_t mutex;
	char activity_id[40];
	char key[8*(8+1)];
	PGconn *conn;
} pg_connection;

// json --------------------------------------------------------------------------------------------

typedef enum json_entry_value_type {
	STRING,
	OBJECT,
	ARRAY
};

typedef struct
{

	int parent;
	enum json_entry_value_type value_type;

	int array_size;
	int array_index;

	int key_begin;
	int key_end;

	int value_begin;
	int value_end;

} json_entry;

typedef struct
{

	json_entry *entries;
	int entries_len;
	int entries_size;

	char *source;

} json;

// http --------------------------------------------------------------------------------------------

#define HTTP_STATUS_OK             200
#define HTTP_STATUS_BAD_REQUEST    400
#define HTTP_STATUS_NOT_FOUND      404
#define HTTP_STATUS_INTERNAL_ERROR 500

#define HTTP_CONTENT_TYPE_HTML        "text/html"
#define HTTP_CONTENT_TYPE_TEXT_PLAIN  "text/plain"
#define HTTP_CONTENT_TYPE_JS          "text/javascript"
#define HTTP_CONTENT_TYPE_JSON        "application/json"

typedef struct
{
	char method[16];
	char path[2048];
	char protocol[16];

	stream content;

} http_request;

#endif /* UTIL_EXT_H_ */
