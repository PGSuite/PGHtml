#include <sys/time.h>

#include <libpq-fe.h>

#include "globals.h"

#include "util/util_str.h"
#include "util/util_file.h"

#define PROGRAM_DESC "PGHtml is utility for creation an html files using data from PostgreSQL"

char HELP[] =
	"Usage:\n" \
	"  pghtml DIRECTORIES [OPTIONS]\n" \
	"\n" \
	"Parameters:\n" \
	"  DIRECTORIES        directories or files separated by space\n" \
	"\n" \
	"File options:\n" \
	"  -e EXTENSIONS      file extensions separated by " FILE_EXTENTIONS_SEPARATOR " (default: " FILE_EXTENTIONS_DEFAULT ")\n" \
	"\n" \
	"Connection options:\n" \
	"  -h HOSTNAME        database server host\n" \
	"  -p PORT            database server port\n" \
	"  -d DBNAME          database name\n" \
	"  -U USERNAME        username (default: current user in operating system)\n" \
	"  -W PASSWORD        user password in PostgreSQL (if nessesary)\n" \
	"  or\n" \
	"  -u URI             database server URI\n" \
	"\n" \
	"Variables options:\n" \
	"  -G_[NAME] VALUE    value of variable [G_NAME] \n" \
	"\n" \
	"Logging options:\n" \
	"  -l  FILE           log file, when specified, the prefix is set to \"ts\"\n" \
	"  -lu FILE           output file for only created and updated destination files\n" \
	"  -lp PREFIX         n - none (default), t - timestamp, ts - \"timestamp stdout|stderr\"\n" \
	"\n" \
	"Info:\n" \
	"  -h, --help         print this help\n"
	"\n" \
	"Examples:\n" \
	"  pghtml /mysite\n" \
	"  pghtml /mysite1 /mysite2/index.pghtml -d sitedb -l /tmp/pghtml.log\n" \
	"  pghtml /mysite -h server-db.mycompany.com -d sitedb -U appuser -W 12345 -lu /tmp/files_updated.lst\n" \
	"  pghtml /mysite -u postgresql://192.168.10.20:5432/sitedb?user=postgres&password=12345 -G_ENV PROD\n";


int main(int argc, char *argv[])
{

	log_initialize("PGHTML-");

	if (argc==1 || strcmp(argv[1],"-h")==0 || strcmp(argv[1],"-help")==0 || strcmp(argv[1],"--help")==0 || strcmp(argv[1],"help")==0) {
		log_stdout_print_header(PROGRAM_DESC);
		log_stdout_printf(HELP);
		exit(0);
	}

	char *log_file   = NULL;
	char *log_prefix = NULL;

	char *db_host             = "";
	char *db_port             = NULL;
	char *db_name             = "";
	char *db_user             = NULL;
	char *db_password         = NULL;
	char db_uri[STR_SIZE_MAX] = "";

	char *extentions  = NULL;

	g_vars.size=0;

	str_list_clear(&directories);

	for(int i=1; i<argc; i++) {
		if (argv[i][0]!='-') {
			if (str_list_add(&directories, argv[i]))
				log_stdout_print_and_exit(2);
			continue;
		}
		if (i==argc-1) {
			log_stderr_print(2, argv[i]);
			log_stdout_print_and_exit(2);
		}
		if (strcmp(argv[i],"-h")==0)   	  db_host=argv[++i];
		else if (strcmp(argv[i],"-p")==0) db_port=argv[++i];
		else if (strcmp(argv[i],"-d")==0) db_name=argv[++i];
		else if (strcmp(argv[i],"-U")==0) db_user=argv[++i];
		else if (strcmp(argv[i],"-W")==0) db_password=argv[++i];
		else if (strcmp(argv[i],"-e")==0) extentions=argv[++i];
		else if (strcmp(argv[i],"-u")==0) {
			if (str_copy(db_uri, sizeof(db_uri), argv[++i]))
				log_stdout_print_and_exit(2);
		}
		else if (strlen(argv[i])>3 && argv[i][1]=='G' && argv[i][2]=='_') {
		    char g_var_name[STR_SIZE_MAX];
		    if (str_substr(g_var_name, sizeof(g_var_name), argv[i], 1, strlen(argv[i])-1))
				log_stdout_print_and_exit(2);
			if (str_map_put(&g_vars, g_var_name, argv[++i]))
				log_stdout_print_and_exit(2);
		}
		else if (strcmp(argv[i],"-l")==0) log_file = argv[++i];
		else if (strcmp(argv[i],"-lp")==0) log_prefix = argv[++i];
		else if (strcmp(argv[i],"-lu")==0) {
			if (file_list_updates_open(argv[++i])) return 1;
		} else {
			log_stderr_print(3, argv[i]);
			log_stdout_print_and_exit(2);
		}
	}

	if (directories.size==0) {
		log_stderr_print(4);
		log_stdout_print_and_exit(2);
	}

    if ( strlen(db_uri)==0 && pg_uri_build(db_uri, sizeof(db_uri), db_host, db_port, db_name, db_user, db_password) )
		log_stdout_print_and_exit(2);

    if (extentions==NULL) extentions = FILE_EXTENTIONS_DEFAULT;
    if (str_list_split(&file_extensions, extentions, FILE_EXTENTIONS_SEPARATOR[0])) return 1;
    for(int i=0; i<file_extensions.size; i++)
    	if (strlen(file_extensions.values[i])<3 || file_extensions.values[i][0]!='p' || file_extensions.values[i][1]!='g') {
    		log_stderr_print(22, file_extensions.values[i]);
    		return 1;
    	}

	log_stdout_print_header(PROGRAM_DESC);

    if (str_list_print(&directories,     PATH_SEPARATOR[0],            "\ndirectories:       ") ||
    	str_list_print(&file_extensions, FILE_EXTENTIONS_SEPARATOR[0], "\nfile extensions:   ")) {
		log_stdout_print_and_exit(2);
    }

    log_stdout_println("global variables:  ");
    for(int i=0; i<g_vars.size; i++)
    	log_stdout_printf("%s%s", i==0 ? "" : ",", g_vars.keys[i]);

    log_stdout_println("libpq version:     %d", PQlibVersion());
    log_stdout_println("");

	if (pg_connect(&pg_conn, db_uri))
		log_stdout_print_and_exit(2);

	int result = process_directories();

	pg_disconnect(pg_conn);

	if (file_list_updates_close()) result=1;

	log_stdout_print_and_exit(result);

}
