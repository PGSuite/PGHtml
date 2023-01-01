#include <libpq-fe.h>

#include "util/utils.h"

#define FILE_EXTENTIONS_SEPARATOR ":"
#define FILE_EXTENTIONS_DEFAULT   "pghtml" FILE_EXTENTIONS_SEPARATOR "pgtxt" FILE_EXTENTIONS_SEPARATOR "pgjs" FILE_EXTENTIONS_SEPARATOR "pgjson"

#define TAG_PGHTML_SQL_BEGIN     "<pghtml-sql>"
#define TAG_PGHTML_SQL_END       "</pghtml-sql>"
#define TAG_PGHTML_INCLUDE_BEGIN "<pghtml-include"
#define TAG_PGHTML_INCLUDE_END   "</pghtml-include>"


extern str_list directories;
extern str_list file_extensions;

extern str_map g_vars;

extern PGconn *pg_conn;
