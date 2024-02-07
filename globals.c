#include <libpq-fe.h>

#include "globals.h"

str_list directories;
str_list file_extensions;

str_map g_vars;

PGconn *pg_conn;

int globals_add_parameters(char *text, int text_size) {
	if (str_add(text, text_size, "Parameters\n", NULL)) return 1;
	if (str_add(text, text_size, "  directories (files):  ", NULL)) return 1;
	for(int i=0; i<directories.len; i++)
		if (str_add(text, text_size, i>0 ? PATH_SEPARATOR: "", directories.values[i], NULL)) return 1;
	if (str_add(text, text_size, "\n  file extensions:      ", NULL)) return 1;
	for(int i=0; i<file_extensions.len; i++)
		if (str_add(text, text_size, i>0 ? FILE_EXTENTIONS_SEPARATOR: "", file_extensions.values[i], NULL)) return 1;
	if (str_add(text, text_size, "\n  global variables:     ", NULL)) return 1;
	for(int i=0; i<g_vars.len; i++)
		if (str_add(text, text_size, i>0 ? "," : "", g_vars.keys[i], "=", g_vars.values[i], NULL)) return 1;
	if (str_add(text, text_size, "\n", NULL)) return 1;
	return 0;
}
