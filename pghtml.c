#include <sys/time.h>

#include <libpq-fe.h>

#include "globals.h"
#include "util/utils.h"

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

	log_set_program_name("PGHtml is utility for creation an html files using data from PostgreSQL", "PGHTML");

	log_check_help(argc, argv, HELP);

	char *log_file   = NULL;

	char *db_host             = "";
	char *db_port             = NULL;
	char *db_name             = "";
	char *db_user             = NULL;
	char *db_password         = NULL;
	char db_uri[STR_SIZE] = "";

	char *extentions  = NULL;

	g_vars.len=0;

	str_list_clear(&directories);

	for(int i=1; i<argc; i++) {
		if (argv[i][0]!='-') {
			if (str_list_add(&directories, argv[i]))
				exit(3);
			continue;
		}
		if (i==argc-1) {
			log_error(2, argv[i]);
			exit(3);
		}
		if (strcmp(argv[i],"-h")==0)   	  db_host=argv[++i];
		else if (strcmp(argv[i],"-p")==0) db_port=argv[++i];
		else if (strcmp(argv[i],"-d")==0) db_name=argv[++i];
		else if (strcmp(argv[i],"-U")==0) db_user=argv[++i];
		else if (strcmp(argv[i],"-W")==0) db_password=argv[++i];
		else if (strcmp(argv[i],"-e")==0) extentions=argv[++i];
		else if (strcmp(argv[i],"-u")==0) {
			if (str_copy(db_uri, sizeof(db_uri), argv[++i]))
				exit(3);
		}
		else if (strlen(argv[i])>3 && argv[i][1]=='G' && argv[i][2]=='_') {
		    char g_var_name[STR_SIZE];
		    if (str_substr(g_var_name, sizeof(g_var_name), argv[i], 1, strlen(argv[i])))
		    	exit(3);
			if (str_map_put(&g_vars, g_var_name, argv[++i]))
				exit(3);
		}
		else if (strcmp(argv[i],"-l")==0) log_file = argv[++i];
		else if (strcmp(argv[i],"-lu")==0) {
			if (file_list_updates_open(argv[++i])) return 1;
		} else {
			log_error(3, argv[i]);
			exit(3);
		}
	}
	if (directories.len==0) {
		log_error(4);
		exit(3);
	}
    if ( strlen(db_uri)==0 && pg_uri_build(db_uri, sizeof(db_uri), db_host, db_port, db_name, db_user, db_password) )
    	exit(3);
    if (extentions==NULL) extentions = FILE_EXTENTIONS_DEFAULT;
    if (str_list_split(&file_extensions, extentions, 0, -1, FILE_EXTENTIONS_SEPARATOR[0])) return 1;
    for(int i=0; i<file_extensions.len; i++)
    	if (strlen(file_extensions.values[i])<3 || file_extensions.values[i][0]!='p' || file_extensions.values[i][1]!='g') {
    		log_error(22, file_extensions.values[i]);
    		exit(3);
    	}

	utils_initialize();

	char header[STR_SIZE];
	if (log_get_header(header, sizeof(header)) || globals_add_parameters(header, sizeof(header))) exit(2);
    log_info("%s", header);

	if (pg_connect(&pg_conn, db_uri))
		log_exit_fatal();

	int result = process_directories();

	pg_disconnect(&pg_conn);

	file_list_updates_close();

    log_info("\n%s", result==0 ? "completed successfully" : "completed with errors");

	exit(result!=0);

}
