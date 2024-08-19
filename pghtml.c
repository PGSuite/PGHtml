#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "util/utils.h"
#include "globals.h"

extern void* admin_server_thread(void *args);
extern void* file_maker_thread(void *args);

char HELP[] =
	"Usage:\n" \
	"  pghtml {COMMAND} [OPTIONS]\n" \
	"\n" \
	"Commands:\n" \
	"  start        start execution in another process\n" \
	"  execute      execute in current process\n" \
	"  status       show runtime status\n" \
	"  stop         stop execution\n" \
	"  sync         only synchronize directory and exit (some options are ignored)\n" \
	"  help|man     print this help\n" \
	"\n" \
	"HTTP options:\n" \
	"  -hd DIRECTORY       site directory (default: " HTTP_DIRECTORY_DEFAULT ")\n" \
	"  -hi INTERVAL        synchronization interval in seconds (default: " HTTP_SYNC_INTERVAL_DEFAULT ")\n" \
	"  -hp PORT            HTTP port (default: " HTTP_PORT_DEFAULT ")\n" \
	"\n" \
	"Connection options:\n" \
	"  -h HOSTNAME        database server host (default: server IP address)\n" \
	"  -p PORT            database server port (default: " DB_PORT_DEFAULT ")\n" \
	"  -d DBNAME          database name (default: " DB_NAME_DEFAULT ")\n" \
	"  -U USERNAME        username for service only (default: " DB_SERVICE_USER_DEFAULT ")\n" \
	"  -W PASSWORD        password of user for service (if nessesary)\n" \
	"\n" \
	"Logging options:\n" \
	"  -l  FILE           log file\n" \
	"\n" \
	"Examples:\n" \
	"  pghtml start -d sitedb -l /var/log/pghtml.log\n" \
	"  pghtml execute -hd /mysite -h server-db.mycompany.com -d sitedb -U admin -W 12345\n" \
    "  pghtml sync -hd /site/db -d sitedb\n";

int main(int argc, char *argv[])
{

	log_set_program_name("PGHtml is HTML template engine using PostgreSQL", "PGHTML");

	log_check_help(argc, argv, HELP);

	if (!strcmp(argv[1],"start")) {

		char *args[20];
		int args_count=0;
		int status;
		int error_code;
		unsigned int pid;
		if (sizeof(args)/sizeof(char *)+4<argc) {
			log_error(81, argc);
			exit(3);
		}
		args[args_count++]=argv[0];
		args[args_count++]="execute";
		int log_set = 0;
		for(int i=2; i<argc; i++) {
			if (!strcmp(argv[i],"-l")) log_set = 1;
			args[args_count++]=argv[i];
		}
		if (!log_set) {
			args[args_count++]="-l";
			args[args_count++]=LOG_FILE_DEFAULT;
		}
		args[args_count++]=NULL;
		char command[32*1024] = "";
		for(int i=0; i<args_count-1; i++)
			if (str_add(command, sizeof(command), i>0 ? " " : "", args[i], NULL)) exit(3);

		#ifdef _WIN32

			STARTUPINFO cif;
			ZeroMemory(&cif,sizeof(STARTUPINFO));
			PROCESS_INFORMATION pi;
			status = !CreateProcess(argv[0], command, NULL,NULL,FALSE,NULL,NULL,NULL,&cif,&pi);
			error_code =  GetLastError();
			pid = pi.hProcess;

		#else

			status = posix_spawn(&pid, args[0], NULL, NULL, args, NULL);

		#endif

		if (status) {
			log_error(36, error_code, command);
			exit(3);
		}
		return 0;
	}


	if (!strcmp(argv[1],"stop") || !strcmp(argv[1],"status")) {
		admin_server_command(argc, argv);
		exit(0);
	}

	if (strcmp(argv[1],"execute") && strcmp(argv[1],"sync")) {
		log_error(39, argv[1]);
		exit(3);
	}

	http_sync_interval = atoi(HTTP_SYNC_INTERVAL_DEFAULT);
	http_port          = atoi(HTTP_PORT_DEFAULT);

	char *db_service_password         = NULL;

	char *log_file   = NULL;

	for(int i=2; i<argc; i++) {
		if (argv[i][0]!='-') {
			log_error(41, argv[i]);
			exit(3);
		}
		if (i==argc-1) {
			log_error(2, argv[i]);
			exit(3);
		}
		if (strcmp(argv[i],"-hd")==0) http_directory=argv[++i];
		else if (strcmp(argv[i],"-hi")==0) {
			http_sync_interval = atoi(argv[++i]);
			if (http_sync_interval<=0) {
				log_error(4, argv[i]);
				exit(3);
			}
		} else if (strcmp(argv[i],"-hp")==0) {
			http_port = atoi(argv[++i]);
			if (http_port<=0) {
				log_error(42, argv[i]);
				exit(3);
			}
		}
		else if (strcmp(argv[i],"-h")==0)  db_host=argv[++i];
		else if (strcmp(argv[i],"-p")==0)  db_port=argv[++i];
		else if (strcmp(argv[i],"-d")==0)  db_name=argv[++i];
		else if (strcmp(argv[i],"-U")==0)  db_service_user=argv[++i];
		else if (strcmp(argv[i],"-W")==0)  db_service_password=argv[++i];
		else if (strcmp(argv[i],"-l")==0)  log_file = argv[++i];
		else {
			log_error(3, argv[i]);
			exit(3);
		}
	}

	admin_port = http_port + ADMIN_PORT_OFFSET;

    if (db_host==NULL) db_host = tcp_host_addr[0]!=0 ? tcp_host_addr : "127.0.0.1";
    db_service_host = !strcmp(db_host,tcp_host_addr) ? "127.0.0.1" : db_host;

	if (pg_uri_build(db_service_uri, sizeof(db_service_uri), db_service_host, db_port, db_name, db_service_user, db_service_password) )
    	exit(3);

	char header[STR_SIZE];
	if (log_get_header(header, sizeof(header))) exit(2);

	if (!strcmp(argv[1],"sync")) {
		log_info("%s", header);
		PGconn *pg_conn;
		int res = pg_connect(&pg_conn, db_service_uri);
		if (!res) {
			res = file_maker_sync_dir(pg_conn);
			pg_disconnect(&pg_conn);
		}
		exit(res);
	}

	if (globals_add_parameters(header, sizeof(header))) exit(2);
	log_info("%s", header);

	utils_initialize(log_file);

	if (tcp_startup()>0) log_exit_fatal();

	if (thread_create(admin_server_thread, "ADMIN_SERVER", NULL))
		log_exit_fatal();

	if (thread_create(file_maker_thread, "FILE_MAKER", NULL))
		log_exit_fatal();

    while(1) sleep(UINT_MAX);

}
