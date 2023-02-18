#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "utils.h"
#include "util_version.h"

#define MSG_EXIT_FATAL "exit due to fatal error"
#define MSG_EXIT_STOP  "exit due to stopping"

typedef enum {LOG_LEVEL_FATAL, LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, LOG_LEVEL_TRACE} log_level;
const char *LOG_LEVEL_NAMES[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
#define LOG_LEVEL_STREAM(level) (level<=1 ? stderr : stdout)

const char *ERRORS[] = {
	"No error",                                                                       //  0
	"Any error (default error code)",                                                 //  1
	"No value for option \"%s\"",                                                     //  2
	"Non-existent option \"%s\"",                                                     //  3
	"Not specified directory for processing",                                         //  4
	"Destination string too small (%d bytes, %d required)",                           //  5
	"Too many attributes for tag \"%s\"",                                             //  6
	"Error parsing HTML tag for position %d",                                         //  7
	"Error open file \"%s\"",                                                         //  8
	"Cannot allocate memory (%d bytes)",                                              //  9
	"Error read file \"%s\"",                                                         // 10
	"File \"%s\" read partially",                                                     // 11
	"Error write file \"%s\"",                                                        // 12
	"File \"%s\" written partially",                                                  // 13
	"Error close file \"%s\"",                                                        // 14
	"Invalid UTF8 first byte (position: %d, text start: \"%.20s\")",                  // 15
	"<empty>", //Too many variables (max %d)",                                                    // 16
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
	"No data to recieve from socket (timeout %d sec)",                                // 33
	"Cannot send to socket (errno %d)",                                               // 34
	"Cannot close socket (errno %d)",                                                 // 35
	"Cannot create process (errno %d), command:\n%s",                                 // 36
	"Cannot lock mutex",                                                              // 37
	"Cannot unlock mutex",                                                            // 38
	"Incorrect administration command \"%s\"",                                        // 39
	"SQL query executed without returning data (query start: \"%.20s\")",             // 40
	"Not option \"%s\"",                                                              // 41
	"Incorrect port number \"%s\"",                                                   // 42
	"Cannot connect to %s:%d (errno %d)",                                             // 43
	"Process not stopped (administration socket not closed)",                         // 44
	"Error on recieved data from socket (errno %d)",                                  // 45
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
	"Too many database сonnections (%d)",                                             // 59
	"Invalid database сonnection id (\"%s\")",                                        // 60
	"Cannot find database сonnection (id: \"%s\")",                                   // 61
	"Database сonnection error: \n%s",                                                // 62
	"JSON array index out of range (index: %d, array size: %d)",                      // 63
	"JSON value type (%d) is not STRING",                                             // 64
	"<empty>", //Too many SQL parameters (%d)",                                                   // 65
	"SQL query returned empty data (query start: \"%.20s\")",                         // 66
	"Invalid request path (\"%s\")",                                                  // 67
	"Too many table columns (%d)",                                                    // 68
	"Too many indexes (%d)",                                                          // 69
	"Too many index columns (%d)",                                                    // 70
	"Cannot mapping type \"%s\" to JavaScript",                                       // 71
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
	"Unrecognized error"                                                              //
};

int          log_initialized = 0;
time_t       log_time_started;
thread_mutex log_mutex;

char log_program_desc[128] = "<program_desc>";
char log_error_prefix[32] = "<ERROR_PREFIX>-";
char log_file_name[STR_SIZE] = "";
FILE log_file;

void log_set_program_name(char *program_desc, char *error_prefix) {
	snprintf(log_program_desc, sizeof(log_program_desc), "%s",  program_desc);
	snprintf(log_error_prefix, sizeof(log_error_prefix), "%s-", error_prefix);
}

int log_get_header(char *header, int header_size) {
	return str_format(header, header_size, "%s\nversion %s, %s %d bits\n\n", log_program_desc, VERSION, OS_NAME, sizeof(void*)*8);
}

void _log_initialize() {
	clock_gettime(0, &log_time_started);
	if (thread_mutex_init(&log_mutex, "log_mutex"))
		log_exit_fatal();
	log_initialized = 1;
}

int log_get_uptime() {
	time_t t;
	time(&t);
	return (int) (t-log_time_started);
}

void _log_print_line_prefix(log_level level) {
	struct timeinfo *timeinfo;
	struct timespec ts;
	char prefix[STR_SIZE];
	clock_gettime(0, &ts);
	timeinfo = localtime(&ts.tv_sec);
	strftime(prefix,sizeof(prefix),"%Y-%m-%d %H:%M:%S",timeinfo);
	thread *thread_current;
	thread_get_current(&thread_current);
	fprintf(LOG_LEVEL_STREAM(level), "%s.%03ld %-5s %-11s  ", prefix, ts.tv_nsec/1000000L, LOG_LEVEL_NAMES[level], thread_current!=NULL ? thread_current->name : "<NOT_FOUND>");
}

void _log_println_str(log_level level, const char *str) {

	FILE *stream = LOG_LEVEL_STREAM(level);

	_log_print_line_prefix(level);

	for(int i=0; str[i]; i++) {
		putc(str[i], stream);
		if (str[i]=='\n')
			_log_print_line_prefix(level);
	}

	putc('\n', stream);

    fflush(stream);

}

void _log_println_fmt(log_level level, const char* format, va_list args) {

	FILE *stream = LOG_LEVEL_STREAM(level);

	char buffer[10*1024];
	int len = vsnprintf(buffer, sizeof(buffer)-50, format, args);

	int buffer_len=strlen(buffer);
	if (len!=buffer_len)
		vsnprintf(buffer+buffer_len, 50, "...[%d more]", len-buffer_len);

	_log_println_str(level, buffer);

}

void log_info(const char* format, ...) {

	thread_mutex_lock(&log_mutex);

    va_list args;
    va_start(args, format);

    _log_println_fmt(LOG_LEVEL_INFO, format, args);

    va_end(args);

	thread_mutex_unlock(&log_mutex);

}

void log_check_help(int argc, char *argv[], char *help) {
	if (argc!=1 && strcmp(argv[1],"-h") && strcmp(argv[1],"-help") && strcmp(argv[1],"--help") && strcmp(argv[1],"help")) return;
	char header[STR_SIZE];
	if (log_get_header(header, sizeof(header))) {
		printf("%s\n%s", header, help);
	}
	exit(0);
}

void log_exit_fatal() {
	if(log_initialized) {
		thread_mutex_lock(&log_mutex);
		_log_println_str(LOG_LEVEL_FATAL, MSG_EXIT_FATAL);
		usleep(1*1000);
	} else {
		fprintf(stderr, "%s\n", MSG_EXIT_FATAL);
	}
	exit(2);
}

void log_exit_stop() {
	thread_mutex_lock(&log_mutex);
	_log_println_str(LOG_LEVEL_INFO, MSG_EXIT_STOP);
	exit(0);
}

// result:
//   1 - always
int log_error(int error_code, ...) {

	int errors_size = sizeof(ERRORS)/sizeof(ERRORS[0]);

	if (error_code<0 || error_code>=errors_size)
		error_code=errors_size-1;

	char error_text[LOG_ERROR_TEXT_SIZE];

	snprintf(error_text, sizeof(error_text), "%s%03d ", log_error_prefix, error_code);
	int prefix_len = strlen(error_text);

    va_list args;
    va_start(args, &error_code);
	vsnprintf(error_text+prefix_len, sizeof(error_text)-prefix_len, ERRORS[error_code], args);
    va_end(args);

	if(!log_initialized) {
		fprintf(stderr, "%s\n", error_text);
		return 1;
	}

	thread_mutex_lock(&log_mutex);
	usleep(3*1000);

	_log_println_str(LOG_LEVEL_ERROR, error_text);

	usleep(3*1000);
    thread_mutex_unlock(&log_mutex);

   	thread_set_last_erorr(error_code, error_text);

   	return 1;
}

void log_set_file(char *value) {
	if (!freopen(value, "a", stdout)) {
		log_info("");
		log_error(25, errno, value, "stdout");
		exit(2);
	}
	fseek(stdout, 0, SEEK_END);
	if (ftello(stdout)!=0) {
		fprintf(stdout, "\n");
	}
	if (!freopen(value, "a", stderr)) {
		log_info(ERRORS[25], errno, value, "stderr");
		exit(2);
	}
	if (str_copy(log_file_name, sizeof(log_file_name), value))
		exit(2);
}
