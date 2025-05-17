#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>

#define MSG_EXIT_FATAL "exit due to fatal error"
#define MSG_EXIT_STOP  "exit due to stopping"

const char *LOG_LEVEL_NAMES[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

const char *LOG_ERRORS[] = {
	"No error",                                                                       // 1000
	"Any error (default error code)",                                                 // 1001
	"No value for option \"%s\"",                                                     // 1002
	"Non-existent option \"%s\"",                                                     // 1003
	"Memory leak detected",                                                           // 1004
	"Destination string too small (%d bytes, %d required)",                           // 1005
	"Too many attributes for tag \"%s\"",                                             // 1006
	"Error parsing HTML tag for position %d",                                         // 1007
	"Error open file \"%s\" (${errno})",                                              // 1008
	"Cannot allocate memory (%d bytes)",                                              // 1009
	"Error read file \"%s\" (${errno})",                                              // 1010
	"File \"%s\" read partially",                                                     // 1011
	"Error write file \"%s\" (${errno})",                                             // 1012
	"File \"%s\" written partially",                                                  // 1013
	"Error close file \"%s\" (${errno})",                                             // 1014
	"Invalid UTF8 first byte (position: %d, text start: \"%.20s\")",                  // 1015
	"Connection denied for user \"%s\"",                                              // 1016
	"Error parsing HTML tag: not found char '%c' for position %d",                    // 1017
	"Error parsing HTML element: not found \"%s\" for position %d",                   // 1018
	"SQL error (%s), query start:\n%.80s\n%s",                                        // 1019
	"List size (%d) too small (value=\"%s\")",                                        // 1020
	"Collection value size (%d) too small for string \"%s...\"",                      // 1021
	"File extension \"%s\" must start with \"pg\"",                                   // 1022
	"Too many directories or files (max %d)",                                         // 1023
	"Map size (%d) too small (key=\"%s\",value=\"%s\")",                              // 1024
	"Error open log file \"%s\" for %s (${errno})",                                   // 1025
	"Cannot create thread (error %d)",                                                // 1026
	"Cannot start WSA (${errno})",                                                    // 1027
	"Cannot create socket (${errno})",                                                // 1028
	"Cannot bind socket to port %d (${errno})",                                       // 1029
	"Cannot listen for incoming connections (${errno})",                              // 1030
	"Cannot accept connection (${errno})",                                            // 1031
	"Cannot set timeout (${errno})",                                                  // 1032
	"Too many table relations (%d)",                                                  // 1033
	"Cannot send to socket (${errno})",                                               // 1034
	"Cannot close socket (${errno})",                                                 // 1035
	"Cannot create process, command:\n%s (${errno})",                                 // 1036
	"Cannot lock mutex",                                                              // 1037
	"Cannot unlock mutex",                                                            // 1038
	"Incorrect execution command \"%s\"",                                             // 1039
	"SQL query executed without returning data (query start: \"%.20s\")",             // 1040
	"Not option \"%s\"",                                                              // 1041
	"Incorrect port number \"%s\"",                                                   // 1042
	"Cannot connect to %s:%d (${errno})",                                             // 1043
	"Process not stopped (administration socket not closed)",                         // 1044
	"Parameter \"%s\" is null",                                                       // 1045
	"Too many (%d) program arguments",                                                // 1046
	"Cannot create process (errno %d), command:\n%s",                                 // 1047
	"Cannot initialize mutex \"%s\"",                                                 // 1048
	"Cannot create directory \"%s\" (${errno})",                                      // 1049
	"Cannot get stat for path \"%s\" (${errno})",                                     // 1050
	"Stream data is null (memory not allocated)",                                     // 1051
	"Unrecognized command (\"%s\")",                                                  // 1052
	"Invalid UTF8 next byte (position: %d, offset: %d, text start: \"%.20s\")",       // 1053
	"Incorrect parameters",                                                           // 1054
	"Cannot parse JSON (error: \"%s\", start position: %d, text: \"%.20s\")",         // 1055
	"JSON is null (memory not allocated)",                                            // 1056
	"Cannot find JSON value (value type: %d, path: \"%s\", start text: \"%.20s\")",   // 1057
	LOG_ERROR_NOT_FOUND_CURRENT_THREAD_TEXT,                                          // 1058
	"Too many database connections (%d)",                                             // 1059
	"Invalid database connection index (\"%d\")",                                     // 1060
	"Cannot find database connection (id: \"%s\")",                                   // 1061
	"Database connection error: \n%s",                                                // 1062
	"JSON array index out of range (index: %d, array size: %d)",                      // 1063
	"JSON value type (%d) is not STRING",                                             // 1064
	"Inappropriate connection key",                                                   // 1065
	"SQL query returned empty data (query start: \"%.20s\")",                         // 1066
	"Invalid request path (\"%s\")",                                                  // 1067
	"Too many table columns (%d)",                                                    // 1068
	"Too many indexes (%d)",                                                          // 1069
	"Too many index columns (%d)",                                                    // 1070
	"Incorrect connection state",                                                     // 1071
	"Cannot parse interval from \"%s\"",                                              // 1072
	"Cannot convert date value \"%.*s\" into ISO8601 string",                         // 1073
	"Cannot convert value \"%s\" into interval",                                      // 1074
	"Too many columns in result query (%d)",                                          // 1075
	"Undefined value function for column \"%s\" (type oid: %d)",                      // 1076
	"SQL query returned more than one row (query start: \"%.20s\")",                  // 1077
	"Cannot remove file \"%s\" (${errno})",                                           // 1078
	"Cannot remove dir \"%s\" (${errno})",                                            // 1079
	"Too long program arguments",                                                     // 1080
	"Too many (%d) program arguments",                                                // 1081
	"Cannot find attribute \"name\" of HTML tag \"pghtml-var\"",                      // 1082
	"Unsupported HTML tag \"%s\"",                                                    // 1083
	"Cannot bind unix socket to path \"%s\" (${errno})",                              // 1084
	"Cannot connect to unix socket \"%s\" (${errno})",                                // 1085
	"Cannot open directory \"%s\" (${errno})",                                        // 1086
	"Error lock file \"%s\" (${errno})",                                              // 1087
	"Error create fork (${errno})",                                                   // 1088
	"Too many command arguments",                                                     // 1089
	"Fork not finished (${errno})",                                                   // 1090
	"Error create pipe (${errno})",                                                   // 1091
	"Error close pipe (${errno})",                                                    // 1092
	"Cannot get IP address for hostname \"%s\" (ai_errno %d,%s)",                     // 1093
	"Cannot rename file \"%s\" to \"%s\" (${errno})",                                 // 1094
	"Cannot initialize semaphore \"%s\"",                                             // 1095
	"Cannot post semaphore",                                                          // 1096
	"Cannot wait semaphore",                                                          // 1097
	"Stream list is full",                                                            // 1098
	"Error convert IP address to text (${errno})",                                    // 1099
	"Parameter \"%s\" has invalid value \"%s\"",                                      // 1100
	"Unrecognized error"                                                              //
};

#define LOG_ERRORS_SIZE   sizeof(LOG_ERRORS)/sizeof(LOG_ERRORS[0])
#define LOG_ERRORS_OFFSET 1000

const char *LOG_WARNINGS[] = {
	"No warning",                                                                     // 9000
	"No data to recieve from socket (timeout %d sec)",                                // 9001
	"Error on recieved data from socket (${errno})",                                  // 9002
	"Unable to get host information (${errno})",                                      // 9003
	"User database connections are made via localhost/127.0.0.1 (may be insecure)",   // 9004
	"OS command executed with error (${errno})\n%soutput:\n%s",                       // 9005
	"Action \"%s\" executed with error",                                              // 9006
	"Update recommended (current version: %s, latest version: %s)",                   // 9007
	"Unrecognized warning"                                                            //
};

#define LOG_WARNINGS_SIZE   sizeof(LOG_WARNINGS)/sizeof(LOG_WARNINGS[0])
#define LOG_WARNINGS_OFFSET 9000


unsigned char  log_initialized    = 0;
unsigned char  log_thread_started = 0;
time_t         log_time_started;
thread_mutex_t log_println_mutex;
thread_mutex_t log_strerror_mutex;

char log_program_name[32]  = "<program_name>";
char log_program_desc[128] = "<program_desc>";

char log_file_name[PATH_MAX] = "";
char log_file_date[20];
int  log_storage_days;

char   log_check_updates;
time_t log_check_updates_time;

void log_set_program_info(char *name, char *desc) {
	snprintf(log_program_name, sizeof(log_program_name), "%s", name);
	snprintf(log_program_desc, sizeof(log_program_desc), "%s", desc);
}

int log_get_program_name(char *name, int name_size) {
	return str_copy(name, name_size, log_program_name);
}

int log_get_header(char *header, int header_size) {
	return str_format(header, header_size, "%s\nversion %s, %s %d bits\n", log_program_desc, VERSION, OS_NAME, sizeof(void*)*8);
}

void log_print_header() {
	char header[STR_SIZE];
	if (log_get_header(header, sizeof(header))) return;
	log_info("%s", header);
}

void log_initialize(char *file_name) {
	clock_gettime(0, &log_time_started);
	thread_mutex_init(&log_println_mutex);
	thread_mutex_init(&log_strerror_mutex);
	if (file_name!=NULL && file_name[0])
		_log_set_file_name(file_name);
	log_initialized = 1;
}

int log_get_uptime() {
	time_t t;
	time(&t);
	return (int) (t-log_time_started);
}

void _log_println_text(log_level level, const char *text, int lock) {
	FILE *stream = (level<=2 && !log_file_name[0] ? stderr : stdout);
	if (!log_initialized) {
		fprintf(stream, "%s\n", text);
		fflush(stream);
		return;
	}
	char prefix[128] = "";
	if (log_file_name[0]) {
		struct timespec ts;
		clock_gettime(0, &ts);
		strftime(prefix,sizeof(prefix),"%Y-%m-%d %H:%M:%S",localtime(&ts.tv_sec));
		str_add_format(prefix, sizeof(prefix), ".%03ld ", ts.tv_nsec/1000000L);
	}
	str_add_format(prefix, sizeof(prefix), "%-5s ", LOG_LEVEL_NAMES[level]);
	thread_add_name(prefix, sizeof(prefix));
	str_add(prefix, sizeof(prefix), "  ", NULL);
	if (lock) thread_mutex_lock(&log_println_mutex);
	for(char *p=text,*p_next;;p=p_next) {
		p_next = strchr(p,'\n');
		fputs(prefix, stream);
		if (p_next==NULL) {
			fputs(p, stream);
			break;
		}
		fwrite(p, 1, ++p_next-p, stream);
	};
	putc('\n', stream);
	if (lock) thread_mutex_unlock(&log_println_mutex);
	if (!log_thread_started) fflush(stream);
}

void log_check_help(int argc, char *argv[], char *help) {
	if (argc!=1  && strcmp(argv[1],"help") && strcmp(argv[1],"man") && strcmp(argv[1],"-h") && strcmp(argv[1],"-help") && strcmp(argv[1],"--help")) return;
	char header[STR_SIZE];
	if (!log_get_header(header, sizeof(header))) {
		printf("%s\n%s", header, help);
	}
	exit(0);
}

void log_exit_fatal() {
	thread_mutex_lock(&log_println_mutex);
	_log_println_text(LOG_LEVEL_FATAL, MSG_EXIT_FATAL, 0);
	exit(2);
}

void log_exit_stop() {
	thread_mutex_lock(&log_println_mutex);
	_log_println_text(LOG_LEVEL_INFO, MSG_EXIT_STOP, 0);
	exit(0);
}

void log_info(const char* format, ...) {
	char text[LOG_TEXT_SIZE];
    va_list args;
    va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
    va_end(args);
	_log_println_text(LOG_LEVEL_INFO, text, 1);
}

char* _log_src_file_base(char *src_file) {
	for(int i=strlen(src_file); i>=0; i--)
		if (src_file[i]=='/' || src_file[i]=='\\' || src_file[i]=='%') return src_file+i+1;
	return src_file;
}

int _log_code(char *src_file, char *src_func, int src_line, log_level level, int code, char *errno_name, int errno_value, ...) {
	char *format =	level==LOG_LEVEL_ERROR ?
		LOG_ERRORS  [LOG_ERRORS_OFFSET  <=code && code<LOG_ERRORS_OFFSET  +LOG_ERRORS_SIZE   ? code-LOG_ERRORS_OFFSET   : LOG_ERRORS_SIZE-1  ]:
		LOG_WARNINGS[LOG_WARNINGS_OFFSET<=code && code<LOG_WARNINGS_OFFSET+LOG_WARNINGS_SIZE ? code-LOG_WARNINGS_OFFSET : LOG_WARNINGS_SIZE-1];
	int pos_errno = str_find(format, 0, "${errno}", 0);
	char format_errno[512];
	if (pos_errno>=0) {
		int len = snprintf(format_errno, sizeof(format_errno), "%.*s%s %d,", pos_errno, format, errno_name==NULL ? "errno" : errno_name, errno_value);
		if (errno_name==NULL && errno_value) {
			if (log_initialized) thread_mutex_lock(&log_strerror_mutex); // strerror_r not found
			char *errno_str = strerror(errno_value);
			for(int i=0;errno_str[i] && len<sizeof(format_errno)-10;i++) {
				format_errno[len++]=errno_str[i];
				if (errno_str[i]=='%') format_errno[len++]=errno_str[i];
			}
			if (log_initialized) thread_mutex_unlock(&log_strerror_mutex);
			format_errno[len]=0;
		}
		snprintf(format_errno+len, sizeof(format_errno)-len, "%s", format+pos_errno+8);
	}
	char text[LOG_TEXT_SIZE];
	int len = snprintf(text, sizeof(text), "PGSUITE-%d [%s:%s:%d] ", code, _log_src_file_base(src_file), src_func, src_line);
    va_list args;
    va_start(args, &errno_value);
	vsnprintf(text+len, sizeof(text)-len, pos_errno>=0 ? format_errno : format, args);
    va_end(args);
	_log_println_text(level, text, 1);
   	return level==LOG_LEVEL_ERROR ? 1 : -1;
}
#ifdef TRACE

void _log_trace(char *src_file, char *src_func, int src_line, const char* format, ...) {
	char src[128];
	snprintf(src, sizeof(src), "[%s:%s:%d]", _log_src_file_base(src_file), src_func, src_line);
	char text[LOG_TEXT_SIZE];
	int prefix_len = snprintf(text, sizeof(text), "%-40s  %02d  ", src, thread_mem_allocated_get());
	va_list args;
    va_start(args, format);
	vsnprintf(text+prefix_len, sizeof(text)-prefix_len, format, args);
    va_end(args);
	_log_println_text(LOG_LEVEL_TRACE, text, 1);
}

#endif

const char *LOG_ERROR_OPEN_FILE = "Error (errno %d) open log file \"%s\" for %s";

void _log_set_file_name(char *file_name) {
	log_info("stdout and stderr is redirected to file %s", file_name);
	if (!freopen(file_name, "a", stdout)) {
		log_info("");
		log_error_errno(1025, errno, file_name, "stdout");
		exit(2);
	}
	fseek(stdout, 0, SEEK_END);
	if (ftello(stdout))
		fprintf(stdout, "\n");
	if (!freopen(file_name, "a", stderr)) {
		log_info(LOG_ERROR_OPEN_FILE, errno, file_name, "stderr");
		exit(2);
	}
	if (
		str_copy(log_file_name, sizeof(log_file_name), file_name) ||
		time_date_str(log_file_date, sizeof(log_file_date), time_now())
	) exit(2);
}

void _log_file_switch() {
	if (!log_file_name[0]) return;
	time_t time = time_now();
	char date[20];
	if(time_date_str(date, sizeof(date), time_now())) return;
	if (strcmp(log_file_date, date)>=0) return;
	char log_file_name_old[PATH_MAX] = "";
	if (str_add(log_file_name_old, sizeof(log_file_name_old), log_file_name, ".", log_file_date, NULL)) return;
	thread_mutex_lock(&log_println_mutex);
	_log_println_text(LOG_LEVEL_INFO, "switching log file", 0);
	#ifdef _WIN32
		fclose(stdout);
		fclose(stderr);
	#endif
	int rename_result = rename(log_file_name, log_file_name_old); int rename_errno = errno;
	int stdout_result=0,stderr_result=0,stdout_errno,stderr_errno;
	#ifdef _WIN32
		stdout_result = freopen(log_file_name, "a", stdout)==NULL; stdout_errno = errno;
		stderr_result = freopen(log_file_name, "a", stderr)==NULL; stderr_errno = errno;
		fseek(stdout, 0, SEEK_END);
	#else
		if (!rename_result) {
			stdout_result = freopen(log_file_name, "w", stdout)==NULL; stdout_errno = errno;
			stderr_result = freopen(log_file_name, "w", stderr)==NULL; stderr_errno = errno;
		}
	#endif
	thread_mutex_unlock(&log_println_mutex);
	if (!rename_result)
		log_info("log file switched, previous renamed to \"%s\"", log_file_name_old);
	else
		log_error_errno(1094, rename_errno, log_file_name, log_file_name_old);
	if (stdout_result) log_error_errno(1025, stderr_errno, log_file_name, "stdout");
	if (stderr_result) log_info(LOG_ERROR_OPEN_FILE, stderr_errno, log_file_name, "stderr");
	if(str_copy(log_file_date, sizeof(log_file_date), date)) log_exit_fatal();
	if (!rename_result)
		_log_file_remove_obsolete(time);
	return;
}

void _log_file_remove_obsolete(time_t time) {
	if (log_storage_days<=0) return;
	char dir_path[PATH_MAX] = "";
	if (file_dir(dir_path, sizeof(dir_path), log_file_name)) return;
	char log_file_date_last[20];
	time_date_str(log_file_date_last, sizeof(log_file_date_last), time-log_storage_days*24*60*60);
	char log_file_name_last[1024] = "";
	if (str_add(log_file_name_last, sizeof(log_file_name_last), log_file_name, ".", log_file_date_last, NULL)) return;
	int log_file_name_last_len = strlen(log_file_name_last);
	DIR *dir = opendir(dir_path);
	if (dir==NULL) { log_error_errno(1086, dir_path); return; }
	struct dirent *dir_ent;
	while ((dir_ent = readdir(dir)) != NULL) {
		char file_name[1024] = "";
		if (str_add(file_name, sizeof(file_name), dir_path, FILE_SEPARATOR, dir_ent->d_name, NULL)) goto final;
		if (
			!strncmp(file_name, log_file_name_last, log_file_name_last_len-8) &&
			strcmp(file_name, log_file_name_last)<0 &&
			!file_remove(file_name, 0)
		)
			log_info("obsolete log file \"%s\" removed", file_name);
	}
final:
	closedir(dir);
}

void _log_check_update() {
	if (!log_check_updates) return;
	time_t time = time_now();
	if (log_check_updates_time+24*60*60>time) return;
	log_check_updates_time = time;
	const char *hostname = "pgsuite.org";
	log_info("check for update, request latest version");
	tcp_socket sock = 0;
	char addr[20];
	if (tcp_addr_hostname(addr, sizeof(addr), hostname)) goto final;
	if (tcp_socket_create(&sock)) goto final;
	if (tcp_connect(sock, addr, 80)) goto final;
	char request[1024];
	if (str_format(request, sizeof(request), "GET /files/version.txt#%s HTTP/1.1\r\nHost: %s\r\n\r\n", log_program_name, hostname)) goto final;
	if (tcp_send(sock, request, strlen(request))) goto final;
	char response[10*1024];
	int response_len=0,pos_data,pos_end;
	char version_latest[20];
	do {
		int recv_len = recv(sock, response+response_len, sizeof(response)-response_len-1, 0);
		if (recv_len<=0) {
			log_warn_errno_tcp(9002, tcp_errno);
			goto final;
		}
		response_len += recv_len;
		response[response_len]=0;
		pos_data = str_find(response, 0, "\r\n\r\n", 0);
		if (pos_data!=-1) {
			pos_data += 4;
			pos_end = str_find(response, pos_data, "\n", 0);
		}
	} while(pos_data==-1 || pos_end==-1);
	if (str_substr(version_latest, sizeof(version_latest), response, pos_data, pos_end-1)) goto final;
	if (!strcmp(version_latest, VERSION))
		log_info("no update required, latest version (%s) is used", version_latest);
	else
		log_warn(9007, VERSION, version_latest);
final:
	tcp_socket_close(sock);
}

void* _log_thread(thread_params_t *params) {
	thread_begin("LOGGER");
	log_thread_started = 1;
	for(int i=1;;sleep(5),i=++i%600) {
		fflush(stdout);
		fflush(stderr);
		if (i) continue;
		_log_file_switch();
		_log_check_update();
	}
	thread_end();
	return 0;
}

int log_thread_create(int storage_days, int check_updates) {
	log_storage_days       = storage_days;
	log_check_updates      = check_updates;
	log_check_updates_time = 0;
	return thread_create(_log_thread, NULL);
}
