#include <libpq-fe.h>

#include "globals.h"

char *http_directory = HTTP_DIRECTORY_DEFAULT;
int  http_sync_interval;
int  http_port;

char *db_host     = NULL;
char *db_port     = DB_PORT_DEFAULT;
char *db_name     = DB_NAME_DEFAULT;
char *db_service_host = NULL;
char *db_service_user = DB_SERVICE_USER_DEFAULT;
char db_service_uri[256];

int admin_port;

int globals_add_parameters(char *text, int text_size) {
	if (str_add(text, text_size, "\nParameters\n", NULL)) return 1;
	if (str_add(text, text_size, "  HTTP\n", NULL)) return 1;
	if (str_add(text, text_size, "    directory:         ", http_directory, "\n", NULL)) return 1;
	if (str_add_format(text, text_size, "    sync interval:     %ds\n", http_sync_interval)) return 1;
	if (str_add_format(text, text_size, "    port:              %d\n", http_port)) return 1;
	if (str_add(text, text_size, "  database\n", NULL)) return 1;
	if (str_add(text, text_size, "    host:              ", db_host,         "\n", NULL)) return 1;
	if (str_add(text, text_size, "    port:              ", db_port,         "\n", NULL)) return 1;
	if (str_add(text, text_size, "    database:          ", db_name,         "\n", NULL)) return 1;
	if (str_add(text, text_size, "    host for service:  ", db_service_host, "\n", NULL)) return 1;
	if (str_add(text, text_size, "    user for service:  ", db_service_user, "\n", NULL)) return 1;
	if (str_add(text, text_size, "  administration\n", NULL)) return 1;
	if (str_add_format(text, text_size, "    port:              %d\n", admin_port)) return 1;
	return 0;
}
