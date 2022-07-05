#include <libpq-fe.h>

#include "globals.h"

char directories[ARRAY_SIZE_MAX][ARRAY_ELEMENT_SIZE_MAX];
int  directories_size;
char file_extensions[ARRAY_SIZE_MAX][ARRAY_ELEMENT_SIZE_MAX];
int  file_extensions_size;

VARS g_vars;



