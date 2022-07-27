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
	"Output options:\n" \
	"  -oi INFO           n - none (only errors)\n" \
	"                     f - only created and updated destination files\n" \
	"                     p - processing (default)\n" \
	"  -op PREFIX         n - none (default), t - timestamp\n" \
	"\n" \
	"Info:\n" \
	"  -h, --help         print this help\n"
	"\n" \
	"Examples:\n" \
	"  pghtml /mysite\n" \
	"  pghtml /mysite1 /mysite2/index.pghtml -d sitedb\n" \
	"  pghtml /mysite -h server-db.mycompany.com -d sitedb -U appuser -W 12345\n" \
	"  pghtml /mysite -u postgresql://192.168.10.20:5432/sitedb?user=postgres&password=12345 -G_ENV PROD -oi f -op t\n";

int main(int argc, char *argv[])
{

	if (argc==1 || strcmp(argv[1],"-h")==0 || strcmp(argv[1],"-help")==0 || strcmp(argv[1],"--help")==0) {
		printf("%s\n", HELP);
		return 1;
	}

	char *pg_host             = "";
	char *pg_port             = NULL;
	char *pg_db_name          = "";
	char *pg_user             = NULL;
	char *pg_password         = NULL;
	char pg_uri[STR_SIZE_MAX] = "";

	char *extentions  = NULL;

	g_vars.size=0;

	directories_size = 0;

	for(int i=1; i<argc; i++) {
		if (argv[i][0]!='-') {
			if (directories_size>=ARRAY_SIZE_MAX) {
				stderr_printf(23, ARRAY_SIZE_MAX);
				strout_print_and_exit();
			}
			if (str_copy(directories[directories_size++], ARRAY_ELEMENT_SIZE_MAX, argv[i]))
				strout_print_and_exit();
			continue;
		}
		if (i==argc-1) {
			stderr_printf(2, argv[i]);
			strout_print_and_exit();
		}
		if (strcmp(argv[i],"-h")==0)   	  pg_host=argv[++i];
		else if (strcmp(argv[i],"-p")==0) pg_port=argv[++i];
		else if (strcmp(argv[i],"-d")==0) pg_db_name=argv[++i];
		else if (strcmp(argv[i],"-U")==0) pg_user=argv[++i];
		else if (strcmp(argv[i],"-W")==0) pg_password=argv[++i];
		else if (strcmp(argv[i],"-e")==0) extentions=argv[++i];
		else if (strcmp(argv[i],"-u")==0) {
			if (str_copy(pg_uri, sizeof(pg_uri), argv[++i]))
				strout_print_and_exit();
		}
		else if (strlen(argv[i])>3 && argv[i][1]=='G' && argv[i][2]=='_') {
		    char g_var_name[STR_SIZE_MAX];
		    if (str_substr(g_var_name, sizeof(g_var_name), argv[i], 1, strlen(argv[i])-1))
				strout_print_and_exit();
			if (str_vars_add(&g_vars, g_var_name, argv[++i]))
				strout_print_and_exit();
		}
		else if (strcmp(argv[i],"-oi")==0) stream_set_output_info_type(argv[++i][0]);
		else if (strcmp(argv[i],"-op")==0) stream_set_output_prefix(argv[++i][0]);
		else {
			stderr_printf(3, argv[i]);
			strout_print_and_exit();
		}
	}

	if (directories_size==0) {
		stderr_printf(4);
		strout_print_and_exit();
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
    if (str_split(file_extensions, &file_extensions_size, extentions, FILE_EXTENTIONS_SEPARATOR[0])) return 1;
    for(int i=0; i<file_extensions_size; i++)
    	if (strlen(file_extensions[i])<3 ||  file_extensions[i][0]!='p' || file_extensions[i][1]!='g') {
    		stderr_printf(22, file_extensions[i]);
    		return 1;
    	}

    stdout_printf('p', "\ndirectories:       ");
    for(int i=0; i<directories_size; i++) {
    	stdout_printf('p', "%s%s", i==0 ? "" : PATH_SEPARATOR, directories[i]);
    }
    stdout_printf('p', "\nfile extensions:   ");
    for(int i=0; i<file_extensions_size; i++)
    	stdout_printf('p', "%s%s", i==0 ? "" : FILE_EXTENTIONS_SEPARATOR, file_extensions[i]);

    stdout_printf('p', "\nglobal variables:  ");
    for(int i=0; i<g_vars.size; i++)
    	stdout_printf('p', "%s%s", i==0 ? "" : ",", g_vars.names[i]);

    stdout_printf('p', "\ndatabase URI:      ");

    int pg_uri_pos = str_find(pg_uri, sizeof(pg_uri), 0, "password=", 1);
    char pg_uri_out[STR_SIZE_MAX];
    if (pg_uri_pos==-1) {
    	stdout_printf('p', "%s", pg_uri);
    } else {
        if (str_substr(pg_uri_out, sizeof(pg_uri_out), pg_uri, 0, pg_uri_pos+9)) return 1;
        stdout_printf('p', "%s%s%s%s", pg_uri_out, "*","*","*");
    	pg_uri_pos = str_find(pg_uri, sizeof(pg_uri), pg_uri_pos, "&", 0);
    	if (pg_uri_pos!=-1) {
            if (str_substr(pg_uri_out, sizeof(pg_uri_out), pg_uri, pg_uri_pos, strlen(pg_uri)-pg_uri_pos)) return 1;
            stdout_printf('p', "%s", pg_uri_out);
    	}
    }

    stdout_printf('p', "\nlibpq version:     %d", PQlibVersion());
    stdout_printf('p', "\n");

	pg_connect(pg_uri);

	int result = process_directories();

	pg_disconnect();

	stdout_printf('p', "\n");
	stdout_printf('p', "\n%s", !result ? "completed successfully" : "completed with errors");
	stdout_printf('p', "\n");

	return result;

}
