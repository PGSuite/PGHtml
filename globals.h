#include <libpq-fe.h>

#include "util/utils.h"

#define FILE_EXTENTIONS_SEPARATOR ":"
#define FILE_EXTENTIONS_DEFAULT   "pghtml" FILE_EXTENTIONS_SEPARATOR "pgtxt" FILE_EXTENTIONS_SEPARATOR "pgjs" FILE_EXTENTIONS_SEPARATOR "pgjson" FILE_EXTENTIONS_SEPARATOR "pgxml"

#define TAG_PGHTML_INCLUDE  "pghtml-include"
#define TAG_PGHTML_VAR      "pghtml-var"
#define TAG_PGHTML_SQL      "pghtml-sql"

extern str_list directories;
extern str_list file_extensions;

extern str_map g_vars;

extern PGconn *pg_conn;
