#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>

#define MSG_EXIT_FATAL "exit due to fatal error"
#define MSG_EXIT_STOP  "exit due to stopping"

typedef enum {LOG_LEVEL_FATAL, LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, LOG_LEVEL_TRACE} log_level;
const char *LOG_LEVEL_NAMES[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

const char *ERRORS[] = {
	"No error",                                                                       //  0
	"Any error (default error code)",                                                 //  1
	"No value for option \"%s\"",                                                     //  2
	"Non-existent option \"%s\"",                                                     //  3
	"Memory leak detected",                                                           //  4
	"Destination string too small (%d bytes, %d required)",                           //  5
	"Too many attributes for tag \"%s\"",                                             //  6
	"Error parsing HTML tag for position %d",                                         //  7
	"Error open file \"%s\" (errno %d)",                                              //  8
	"Cannot allocate memory (%d bytes)",                                              //  9
	"Error read file \"%s\"",                                                         // 10
	"File \"%s\" read partially",                                                     // 11
	"Error write file \"%s\"",                                                        // 12
	"File \"%s\" written partially",                                                  // 13
	"Error close file \"%s\"",                                                        // 14
	"Invalid UTF8 first byte (position: %d, text start: \"%.20s\")",                  // 15
	"Connection denied for user \"%s\"",                                              // 16
	"Error parsing HTML tag: not found char '%c' for position %d",                    // 17
	"Error parsing HTML element: not found \"%s\" for position %d",                   // 18
	"SQL error (%s), query start:\n%.80s\n%s",                                        // 19
	"List size (%d) too small (value=\"%s\")",                                        // 20
	"Collection value size (%d) too small for string \"%s...\"",                      // 21
	"File extension \"%s\" must start with \"pg\"",                                   // 22
	"Too many directories or files (max %d)",                                         // 23
	"Map size (%d) too small (key=\"%s\",value=\"%s\")",                              // 24
	"Error (errno %d) open log file \"%s\" for %s",                                   // 25
	"Cannot create thread \"%s\"",                                                    // 26
	"Cannot start WSA (errno %d)",                                                    // 27
	"Cannot create socket (errno %d)",                                                // 28
	"Cannot bind socket to port %d (errno %d)",                                       // 29
	"Cannot listen for incoming connections (errno %d)",                              // 30
	"Cannot accept connection (errno %d)",                                            // 31
	"Cannot set timeout (errno %d)",                                                  // 32
	"Too many table relations (%d)",                                                  // 33
	"Cannot send to socket (errno %d)",                                               // 34
	"Cannot close socket (errno %d)",                                                 // 35
	"Cannot create process (errno %d), command:\n%s",                                 // 36
	"Cannot lock mutex",                                                              // 37
	"Cannot unlock mutex",                                                            // 38
	"Incorrect execution command \"%s\"",                                             // 39
	"SQL query executed without returning data (query start: \"%.20s\")",             // 40
	"Not option \"%s\"",                                                              // 41
	"Incorrect port number \"%s\"",                                                   // 42
	"Cannot connect to %s:%d (errno %d)",                                             // 43
	"Process not stopped (administration socket not closed)",                         // 44
	"Parameter \"%s\" is null (%s)",                                                  // 45
	"Too many (%d) program arguments",                                                // 46
	"Cannot create process (errno %d), command:\n%s",                                 // 47
	"Cannot initialize mutex \"%s\"",                                                 // 48
	"Cannot create directory \"%s\" (errno %d)",                                      // 49
	"Cannot get stat for path \"%s\" (errno %d)",                                     // 50
	"Stream data is null (memory not allocated)",                                     // 51
	"Unrecognized command (\"%s\")",                                                  // 52
	"Invalid UTF8 next byte (position: %d, offset: %d, text start: \"%.20s\")",       // 53
	"Incorrect parameters (%s)",                                                      // 54
	"Cannot parse JSON (error: \"%s\", start position: %d, text: \"%.20s\")",         // 55
	"JSON is null (memory not allocated)",                                            // 56
	"Cannot find JSON value (value type: %d, path: \"%s\", start text: \"%.20s\")",   // 57
	LOG_ERROR_NOT_FOUND_CURRENT_THREAD_TEXT,                                          // 58
	"Too many database connections (%d)",                                             // 59
	"Invalid database connection index (\"%d\")",                                     // 60
	"Cannot find database connection (id: \"%s\")",                                   // 61
	"Database connection error: \n%s",                                                // 62
	"JSON array index out of range (index: %d, array size: %d)",                      // 63
	"JSON value type (%d) is not STRING",                                             // 64
	"Inappropriate connection key",                                                   // 65
	"SQL query returned empty data (query start: \"%.20s\")",                         // 66
	"Invalid request path (\"%s\")",                                                  // 67
	"Too many table columns (%d)",                                                    // 68
	"Too many indexes (%d)",                                                          // 69
	"Too many index columns (%d)",                                                    // 70
	"Incorrect connection state",                                                     // 71
	"Cannot parse interval from \"%s\"",                                              // 72
	"Cannot convert date value \"%.*s\" into ISO8601 string",                         // 73
	"Cannot convert value \"%s\" into interval",                                      // 74
	"Too many columns in result query (%d)",                                          // 75
	"Undefined value function for column \"%s\" (type oid: %d)",                      // 76
	"SQL query returned more than one row (query start: \"%.20s\")",                  // 77
	"Cannot remove file \"%s\" (errno %d)",                                           // 78
	"Cannot remove dir \"%s\" (errno %d)",                                            // 79
	"Too long program arguments",                                                     // 80
	"Too many (%d) program arguments",                                                // 81
	"Cannot find attribute \"name\" of HTML tag \"pghtml-var\"",                      // 82
	"Unsupported HTML tag \"%s\"",                                                    // 83
	"Cannot bind unix socket to path \"%s\" (errno %d)",                              // 84
	"Cannot connect to unix socket \"%s\" (errno %d)",                                // 85
	"Cannot open directory \"%s\" (errno %d)",                                        // 86
	"Error lock file \"%s\" (errno %d)",                                              // 87
	"Error create fork (errno %d)",                                                   // 88
	"Too many command arguments",                                                     // 89
	"Fork not finished (errno %d)",                                                   // 90
	"Error create pipe (errno %d)",                                                   // 91
	"Error close pipe (errno %d)",                                                    // 92
	"Cannot get IP address for hostname \"%s\" (%s %d)",                              // 93
	"Cannot rename file \"%s\" to \"%s\" (errno %d)",                                 // 94
	"Unrecognized error"                                                              //
};

const char *WARNINGS[] = {
	"No warning",                                                                     // 900
	"No data to recieve from socket (timeout %d sec)",                                // 901
	"Error on recieved data from socket (errno %d)",                                  // 902
	"Unable to get host information (errno %d)",                                      // 903
	"User database connections are made via localhost/127.0.0.1 (may be insecure)",   // 904
	"OS command executed with error (errno: %d)\n%soutput:\n%s",                      // 905
	"Action \"%s\" executed with error",                                              // 906
	"Update recommended (current version: %s, latest version: %s)",                   // 907
	"Unrecognized warning"                                                            //
};

unsigned char log_initialized    = 0;
unsigned char log_thread_started = 0;
time_t        log_time_started;
int           log_thread_name_len;
thread_mutex  log_mutex;

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


void log_initialize2(char *file_name, int thread_name_len) {
	clock_gettime(0, &log_time_started);
	if (thread_mutex_init(&log_mutex, "log_mutex"))
		log_exit_fatal();
	if (file_name!=NULL && file_name[0])
		_log_set_file_name(file_name);
	log_thread_name_len = thread_name_len;
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
	char prefix[128];
	int prefix_len;
	if (log_file_name[0]) {
		struct timespec ts;
		clock_gettime(0, &ts);
		prefix_len = strftime(prefix,sizeof(prefix),"%Y-%m-%d %H:%M:%S",localtime(&ts.tv_sec));
		prefix_len += snprintf(prefix+prefix_len, sizeof(prefix)-prefix_len, ".%03ld ", ts.tv_nsec/1000000L);
	} else {
		prefix[0]  = 0;
		prefix_len = 0;
	}
	thread *thread_current;
	thread_get_current(&thread_current);
	snprintf(prefix+prefix_len, sizeof(prefix)-prefix_len, "%-5s %-*s  ", LOG_LEVEL_NAMES[level], log_thread_name_len, thread_current!=NULL ? thread_current->name : "");
	if (lock) thread_mutex_lock(&log_mutex);
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
	if (lock) thread_mutex_unlock(&log_mutex);
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
	thread_mutex_lock(&log_mutex);
	_log_println_text(LOG_LEVEL_FATAL, MSG_EXIT_FATAL, 0);
	exit(2);
}

void log_exit_stop() {
	thread_mutex_lock(&log_mutex);
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

// result:
//   1 - always
int log_error(int error_code, ...) {
	int errors_size = sizeof(ERRORS)/sizeof(ERRORS[0]);
	if (error_code<0 || error_code>=errors_size)
		error_code=errors_size-1;
	char text[LOG_TEXT_SIZE];
	snprintf(text, sizeof(text), "PGSUITE-%03d ", error_code);
	int prefix_len = strlen(text);
    va_list args;
    va_start(args, &error_code);
	vsnprintf(text+prefix_len, sizeof(text)-prefix_len, ERRORS[error_code], args);
    va_end(args);
	_log_println_text(LOG_LEVEL_ERROR, text, 1);
	thread_set_last_erorr(error_code, text);
   	return 1;
}

// result:
//   -1 - always
int log_warn(int warning_code, ...) {
	int warnings_size = sizeof(WARNINGS)/sizeof(WARNINGS[0]);
	if ((warning_code-900)<0 || (warning_code-900)>=warnings_size)
		warning_code=900+warnings_size-1;
	char text[LOG_TEXT_SIZE];
	snprintf(text, sizeof(text), "PGSUITE-%03d ", warning_code);
	int prefix_len = strlen(text);
    va_list args;
    va_start(args, &warning_code);
	vsnprintf(text+prefix_len, sizeof(text)-prefix_len, WARNINGS[warning_code-900], args);
    va_end(args);
	_log_println_text(LOG_LEVEL_WARN, text, 1);
   	return -1;
}

#ifdef TRACE

void _log_trace(char *src_func, int src_line, const char* format, ...) {
	char src[128];
	snprintf(src, sizeof(src), "%s:%d", src_func, src_line);
	char text[LOG_TEXT_SIZE];
	int prefix_len = snprintf(text, sizeof(text), "%-30s  ", src);
	thread *thread_current;
	if (threads_initialized && !thread_get_current(&thread_current))
		prefix_len += snprintf(text+prefix_len, sizeof(text)-prefix_len, "%02d  ", thread_current->mem_allocated);
	va_list args;
    va_start(args, format);
	vsnprintf(text+prefix_len, sizeof(text)-prefix_len, format, args);
    va_end(args);
	_log_println_text(LOG_LEVEL_TRACE, text, 1);
}

#endif

void _log_set_file_name(char *file_name) {
	log_info("stdout and stderr is redirected to file %s", file_name);
	if (!freopen(file_name, "a", stdout)) {
		log_info("");
		log_error(25, errno, file_name, "stdout");
		exit(2);
	}
	fseek(stdout, 0, SEEK_END);
	if (ftello(stdout))
		fprintf(stdout, "\n");
	if (!freopen(file_name, "a", stderr)) {
		log_info(ERRORS[25], errno, file_name, "stderr");
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
	thread_mutex_lock(&log_mutex);
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
	thread_mutex_unlock(&log_mutex);
	if (!rename_result)
		log_info("log file switched, previous renamed to \"%s\"", log_file_name_old);
	else
		log_error(94, log_file_name, log_file_name_old, rename_errno);
	if (stdout_result) log_error(25,         stdout_errno, log_file_name, "stdout");
	if (stderr_result) log_info (ERRORS[25], stderr_errno, log_file_name, "stderr");
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
	if (dir==NULL) { log_error(86, dir_path, errno); return; }
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
			log_warn(902, tcp_errno);
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
		log_warn(907, VERSION, version_latest);
final:
	tcp_socket_close(sock);
}

void* _log_thread(void *args) {
	thread_begin(args);
	log_thread_started = 1;
	for(int i=1;;i=++i%60) {
		sleep(10);
		fflush(stdout);
		fflush(stderr);
		if (i) continue;
		_log_file_switch();
		_log_check_update();
	}
	thread_end(args);
	return 0;
}

int log_thread_create(int storage_days, int check_updates) {
	log_storage_days       = storage_days;
	log_check_updates      = check_updates;
	log_check_updates_time = 0;
	return thread_create(_log_thread, "LOGGER", NULL);
}
