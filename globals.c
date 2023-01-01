#include <libpq-fe.h>

#include "globals.h"

str_list directories;
str_list file_extensions;

str_map g_vars;

PGconn *pg_conn;
