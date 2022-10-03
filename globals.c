#include <libpq-fe.h>

#include "globals.h"

STR_LIST directories;
STR_LIST file_extensions;

STR_MAP g_vars;

PGconn *pg_conn;
