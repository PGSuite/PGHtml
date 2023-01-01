#ifndef UTILS_H_
#define UTILS_H_

// file --------------------------------------------------------------------------------------------

#ifdef _WIN32
#define FILE_SEPARATOR  "\\"
#define PATH_SEPARATOR  ";"
#define OS_NAME         "windows"
#else
#define FILE_SEPARATOR  "/"
#define PATH_SEPARATOR  ":"
#define OS_NAME         "linux"
#endif


// log ---------------------------------------------------------------------------------------------

#define LOG_ERROR_TEXT_SIZE 512

#define LOG_ERROR_NOT_FOUND_CURRENT_THREAD_CODE 58
#define LOG_ERROR_NOT_FOUND_CURRENT_THREAD_TEXT "Not found current thread"

// tcp ---------------------------------------------------------------------------------------------

#ifdef _WIN32

#include <winsock2.h>
#define tcp_socket SOCKET
#define tcp_errno  WSAGetLastError()

#else

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#define tcp_socket int
#define tcp_errno  errno

#endif

#define TCP_TIMEOUT 5


// stream ---------------------------------------------------------------------------------------

typedef struct
{
	int	 len;
	int	 size;
	char *data;
} stream;


// http --------------------------------------------------------------------------------------------

#define HTTP_STATUS_OK             200
#define HTTP_STATUS_BAD_REQUEST    400
#define HTTP_STATUS_NOT_FOUND      404
#define HTTP_STATUS_INTERNAL_ERROR 500

#define HTTP_CONTENT_TYPE_HTML "text/html"
#define HTTP_CONTENT_TYPE_JS   "text/javascript"
#define HTTP_CONTENT_TYPE_JSON "application/json"

typedef struct
{
	char method[16];
	char path[2048];
	char protocol[16];

	stream content;

} http_request;


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


// thread ------------------------------------------------------------------------------------------

#ifdef _WIN32

#include <processthreadsapi.h>
#define thread_sys_id HANDLE
#define thread_mutex  HANDLE

#else

#include <pthread.h>
#define thread_sys_id pthread_t
#define thread_mutex  pthread_mutex_t

#endif

#define THREAD_NAME_SIZE  20

typedef struct
{

	int used;
	thread_sys_id sys_id;
	unsigned int id;
	char name[THREAD_NAME_SIZE];
	tcp_socket socket_connection;
	int last_error_code;
	char last_error_text[LOG_ERROR_TEXT_SIZE];

} thread;


// str ---------------------------------------------------------------------------------------------

#include <string.h>

#define STR_SIZE 1024

#define STR_COLLECTION_SIZE        30
#define STR_COLLECTION_KEY_SIZE    50
#define STR_COLLECTION_VALUE_SIZE 300

typedef struct
{
	int	 len;
	char keys  [STR_COLLECTION_SIZE][STR_COLLECTION_KEY_SIZE];
	char values[STR_COLLECTION_SIZE][STR_COLLECTION_VALUE_SIZE];
} str_map;


typedef struct
{
	int	 len;
	char values[STR_COLLECTION_SIZE][STR_COLLECTION_VALUE_SIZE];
} str_list;


// pg ----------------------------------------------------------------------------------------------

#include <libpq-fe.h>

#define PG_CLIENT_ENCODING "UTF8"

#define PG_CONNECTIONS_SIZE  1000
#define PG_SQL_PARAMS_SIZE   256

#define PG_SQL_SESSION_ID_COLUMN   "upper(md5(pid||'-'||to_char(backend_start, 'yyyymmdd-hh24miss-us'))) session_id"

typedef struct
{

	int assigned;
	thread_mutex mutex;
	char id[50];
	PGconn *conn;

} pg_connection;

typedef struct
{

	stream streams[PG_SQL_PARAMS_SIZE];
	char *data[PG_SQL_PARAMS_SIZE];
	int len;

} pg_sql_params;



#endif /* UTILS_H_ */
