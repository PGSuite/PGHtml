#include <libpq-fe.h>

#include "util/util.h"

#define TAG_PGHTML_INCLUDE  "pghtml-include"
#define TAG_PGHTML_VAR      "pghtml-var"

#ifdef _WIN32

#define HTTP_DIRECTORY_DEFAULT "C:\\Site"
#define LOG_FILE_DEFAULT       "C:\\Site\\log\\pghtml.log"

#else

#define HTTP_DIRECTORY_DEFAULT "/site"
#define LOG_FILE_DEFAULT       "/var/log/pgsuite/pghtml.log"

#endif

#define HTTP_SYNC_INTERVAL_DEFAULT  "600"
#define HTTP_PORT_DEFAULT           "5480"

#define DB_PORT_DEFAULT          "5432"
#define DB_NAME_DEFAULT          "site"
#define DB_SERVICE_USER_DEFAULT  "postgres"

extern char *db_host;
extern char *db_port;
extern char *db_name;
extern char *db_service_host;
extern char *db_service_user;
extern char db_service_uri[256];

extern char *http_directory;
extern int  http_sync_interval;
extern int  http_port;
