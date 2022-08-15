#include <sys/time.h>

#include <libpq-fe.h>

#include "globals.h"

#include "util/util-str.h"
#include "util/util-file.h"

char HELP[] =
	"pghtml is utility for creation an html files using data from PostgreSQL\n" \
	"\n" \
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
	"  -lf FILE           log file, when specified, the prefix is ​​set to \"tsp\"\n" \
	"  -li INFO           n - none (only errors)\n" \
	"                     f - only created and updated destination files\n" \
	"                     p - processing (default)\n" \
	"  -lp PREFIX         n - none (default), t - timestamp, tsp - \"timestamp stdout|stderr pthread_id\"\n" \
	"\n" \
	"Info:\n" \
	"  --help         print this help\n"
	"\n" \
	"Examples:\n" \
	"  pghtml /mysite\n" \
	"  pghtml /mysite1 /mysite2/index.pghtml -d sitedb -lf /tmp/pghtml.log\n" \
	"  pghtml /mysite -h server-db.mycompany.com -d sitedb -U appuser -W 12345\n" \
	"  pghtml /mysite -u postgresql://192.168.10.20:5432/sitedb?user=postgres&password=12345 -G_ENV PROD -pi f -lp t\n";

int main(int argc, char *argv[])
{

	if (argc==1 || strcmp(argv[1],"-help")==0 || strcmp(argv[1],"--help")==0) {
		printf("%s\n", HELP);
		return 1;
	}

	log_set_program_name("PGHtml");
	log_set_error_prefix("PGHTML-");

	char *pg_host             = "";
	char *pg_port             = NULL;
	char *pg_db_name          = "";
	char *pg_user             = NULL;
	char *pg_password         = NULL;
	char pg_uri[STR_SIZE_MAX] = "";

	char *extentions  = NULL;

	g_vars.size=0;

	str_list_clear(&directories);

	for(int i=1; i<argc; i++) {
		if (argv[i][0]!='-') {
			if (str_list_add(&directories, argv[i]))
				log_stdout_print_and_exit();
			continue;
		}
		if (i==argc-1) {
			log_stderr_printf(2, argv[i]);
			log_stdout_print_and_exit();
		}
		if (strcmp(argv[i],"-h")==0)   	  pg_host=argv[++i];
		else if (strcmp(argv[i],"-p")==0) pg_port=argv[++i];
		else if (strcmp(argv[i],"-d")==0) pg_db_name=argv[++i];
		else if (strcmp(argv[i],"-U")==0) pg_user=argv[++i];
		else if (strcmp(argv[i],"-W")==0) pg_password=argv[++i];
		else if (strcmp(argv[i],"-e")==0) extentions=argv[++i];
		else if (strcmp(argv[i],"-u")==0) {
			if (str_copy(pg_uri, sizeof(pg_uri), argv[++i]))
				log_stdout_print_and_exit();
		}
		else if (strlen(argv[i])>3 && argv[i][1]=='G' && argv[i][2]=='_') {
		    char g_var_name[STR_SIZE_MAX];
		    if (str_substr(g_var_name, sizeof(g_var_name), argv[i], 1, strlen(argv[i])-1))
				log_stdout_print_and_exit();
			if (str_map_put(&g_vars, g_var_name, argv[++i]))
				log_stdout_print_and_exit();
		}
		else if (strcmp(argv[i],"-li")==0) log_set_info_type(argv[++i][0]);
		else if (strcmp(argv[i],"-lp")==0) log_set_prefix(argv[++i][0]);
		else {
			log_stderr_printf(3, argv[i]);
			log_stdout_print_and_exit();
		}
	}

	if (directories.size==0) {
		log_stderr_printf(4);
		log_stdout_print_and_exit();
	}

    if (strlen(pg_uri)==0) {
        if (str_add_n(pg_uri, sizeof(pg_uri), 2, "postgresql://", pg_host)) return 1;
        if (pg_port!=NULL)
        	if (str_add_n(pg_uri, sizeof(pg_uri), 2, ":", pg_port)) return 1;
        if (str_add_n(pg_uri, sizeof(pg_uri), 3, "/", pg_db_name, "?connect_timeout=10")) return 1;
        if (pg_user!=NULL      && str_add_n(pg_uri, sizeof(pg_uri), 2, "&user=", pg_user)) return 1;
        if (pg_password !=NULL && str_add_n(pg_uri, sizeof(pg_uri), 2, "&password=", pg_password)) return 1;
	}

    if (extentions==NULL) extentions = FILE_EXTENTIONS_DEFAULT;
    if (str_list_split(&file_extensions, extentions, FILE_EXTENTIONS_SEPARATOR[0])) return 1;
    for(int i=0; i<file_extensions.size; i++)
    	if (strlen(file_extensions.values[i])<3 || file_extensions.values[i][0]!='p' || file_extensions.values[i][1]!='g') {
    		log_stderr_printf(22, file_extensions.values[i]);
    		return 1;
    	}

    log_stdout_printf('p', "\ndirectories:       ");
    for(int i=0; i<directories.size; i++) {
    	log_stdout_printf('p', "%s%s", i==0 ? "" : PATH_SEPARATOR, directories.values[i]);
    }
    log_stdout_printf('p', "\nfile extensions:   ");
    for(int i=0; i<file_extensions.size; i++)
    	log_stdout_printf('p', "%s%s", i==0 ? "" : FILE_EXTENTIONS_SEPARATOR, file_extensions.values[i]);

    log_stdout_printf('p', "\nglobal variables:  ");
    for(int i=0; i<g_vars.size; i++)
    	log_stdout_printf('p', "%s%s", i==0 ? "" : ",", g_vars.keys[i]);

    log_stdout_printf('p', "\ndatabase URI:      ");

    int pg_uri_pos = str_find(pg_uri, sizeof(pg_uri), 0, "password=", 1);
    char pg_uri_out[STR_SIZE_MAX];
    if (pg_uri_pos==-1) {
    	log_stdout_printf('p', "%s", pg_uri);
    } else {
        if (str_substr(pg_uri_out, sizeof(pg_uri_out), pg_uri, 0, pg_uri_pos+9)) return 1;
        log_stdout_printf('p', "%s%s%s%s", pg_uri_out, "*","*","*");
    	pg_uri_pos = str_find(pg_uri, sizeof(pg_uri), pg_uri_pos, "&", 0);
    	if (pg_uri_pos!=-1) {
            if (str_substr(pg_uri_out, sizeof(pg_uri_out), pg_uri, pg_uri_pos, strlen(pg_uri)-pg_uri_pos)) return 1;
            log_stdout_printf('p', "%s", pg_uri_out);
    	}
    }

    log_stdout_printf('p', "\nlibpq version:     %d", PQlibVersion());
    log_stdout_printf('p', "\n");

	pg_connect(pg_uri);

	int result = process_directories();

	pg_disconnect();

	log_stdout_printf('p', "\n");
	log_stdout_printf('p', "\n%s", !result ? "completed successfully" : "completed with errors");
	log_stdout_printf('p', "\n");

	return result;

}
