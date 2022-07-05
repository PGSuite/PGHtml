#include <libpq-fe.h>

#include "util/util-str.h"

#define FILE_EXTENTIONS_SEPARATOR ":"
#define FILE_EXTENTIONS_DEFAULT   "pghtml" FILE_EXTENTIONS_SEPARATOR "pgtxt" FILE_EXTENTIONS_SEPARATOR "pgjs" FILE_EXTENTIONS_SEPARATOR "pgjson"

#define TAG_PGHTML_SQL_BEGIN     "<pghtml-sql>"
#define TAG_PGHTML_SQL_END       "</pghtml-sql>"
#define TAG_PGHTML_INCLUDE_BEGIN "<pghtml-include"
#define TAG_PGHTML_INCLUDE_END   "</pghtml-include>"


extern char directories[ARRAY_SIZE_MAX][ARRAY_ELEMENT_SIZE_MAX];
extern int  directories_size;
extern char file_extensions[ARRAY_SIZE_MAX][ARRAY_ELEMENT_SIZE_MAX];
extern int  file_extensions_size;

extern VARS g_vars;

