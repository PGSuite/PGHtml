#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "utils.h"
#include "util_version.h"

#define STDTYPE_OUT             1
#define STDTYPE_ERR             2
#define STDTYPE_STREAM(stdtype) (stdtype==STDTYPE_OUT ? stdout : stderr)
#define THREAD_NAME_FORMAT      "%-11s "

const char *ERRORS[] = {
	"No error",                                                                       //  0
	"Any error (default error code)",                                                 //  1
	"No value for option \"%s\"",                                                     //  2
	"Non-existent option \"%s\"",                                                     //  3
	"Not specified directory for processing",                                         //  4
	"Destination string too small (%d bytes, %d required)",                           //  5
	"Too many attributes for tag \"%s\"",                                             //  6
	"Not parse attribute name from position %d of tag \"%s\"",                        //  7
	"Error open file \"%s\"\n",                                                       //  8
	"Can not allocate memory (%d bytes)",                                             //  9
	"Error read file \"%s\"",                                                         // 10
	"File \"%s\" read partially",                                                     // 11
	"Error write file \"%s\"",                                                        // 12
	"File \"%s\" written partially",                                                  // 13
	"Error close file \"%s\"",                                                        // 14
	"Invalid UTF8 first byte (position: %d, text start: \"%.20s\")",                  // 15
	"Not found \"%s\" for position %d",                                               // 16
	"Not found char \"}\" for start position %d",                                     // 17
	"Not found tag \"%s\" for start position %d",                                     // 18
	"SQL error (%s), query start:\n%.80s\n%s",                                        // 19
	"List size (%d) too small (value=\"%s\")",                                        // 20
	"Collection value size (%d) too small for string \"%s...\"",                      // 21
	"File extension \"%s\" must start with \"pg\"",                                   // 22
	"Too many directories or files (max %d)",                                         // 23
	"Map size (%d) too small (key=\"%s\",value=\"%s\")",                              // 24
	"Error (errno %d) open log file \"%s\" for %s",                                   // 25
	"Can not create thread \"%s\"",                                                   // 26
	"Can not start WSA (errno %d)",                                                   // 27
	"Can not create socket (errno %d)",                                               // 28
	"Can not bind socket to port %d (errno %d)",                                      // 29
	"Can not listen for incoming connections (errno %d)",                             // 30
	"Can not accept connection (errno %d)",                                           // 31
	"Can not set timeout (errno %d)",                                                 // 32
	"No data to recieve from socket (timeout %d sec)",                                // 33
	"Can not send to socket (errno %d)",                                              // 34
	"Can not close socket (errno %d)",                                                // 35
	"Can not initialize mutex",                                                       // 36
	"Can not lock mutex",                                                             // 37
	"Can not unlock mutex",                                                           // 38
	"Incorrect administration command \"%s\"",                                        // 39
	"SQL query executed without returning data (query start: \"%.20s\")",             // 40
	"Not option \"%s\"",                                                              // 41
	"Incorrect port number \"%s\"",                                                   // 42
	"Can not connect to %s:%d (errno %d)",                                            // 43
	"Process not stopped (administration socket not closed)",                         // 44
	"Error on recieved data from socket (errno %d)",                                  // 45
	"Too many (%d) program arguments",                                                // 46
	"Can not create process (errno %d), command:\n%s",                                // 47
	"Can not initialize mutex \"%s\"",                                                // 48
	"Can not create directory \"%s\" (errno %d)",                                     // 49
	"Can not get stat for path \"%s\" (errno %d)",                                    // 50
	"Stream data is null (memory not allocated)",                                     // 51
	"Unrecognized command (\"%s\")",                                                  // 52
	"Invalid UTF8 next byte (position: %d, offset: %d, text start: \"%.20s\")",       // 53
	"Incorrect parameters (%s)",                                                      // 54
	"Can not parse JSON (error: \"%s\", start position: %d, text: \"%.20s\")",        // 55
	"JSON is null (memory not allocated)",                                            // 56
	"Can not find JSON value (value type: %d, path: \"%s\", start text: \"%.20s\")",  // 57
	LOG_ERROR_NOT_FOUND_CURRENT_THREAD_TEXT,                                          // 58
	"Too many database сonnections (%d)",                                             // 59
	"Invalid database сonnection id (\"%s\")",                                        // 60
	"Can not find database сonnection (id: \"%s\")",                                  // 61
	"Database сonnection error: \n%s",                                                // 62
	"JSON array index out of range (index: %d, array size: %d)",                      // 63
	"JSON value type (%d) is not STRING",                                             // 64
	"Too many SQL parameters (%d)",                                                   // 65
	"SQL query return empty data (query start: \"%.20s\")",                           // 66
	"Invalid request path (\"%s\")",                                                  // 67
	"Too many table columns (%d)",                                                    // 68
	"Too many indexes (%d)",                                                          // 69
	"Too many index columns (%d)",                                                    // 70
	"Can not mapping type \"%s\" to JavaScript",                                      // 71
	"Can not parse interval from \"%s\"",                                             // 72
	"Can not convert date value \"%.*s\" into ISO8601 string",                        // 73
	"Can not convert value \"%s\" into interval",                                     // 74
	"Too many columns in result query (%d)",                                          // 75
	"Undefined value function for column \"%s\" (type oid: %d)",                      // 76
	"Unrecognized error"                                                              //
};

time_t log_time_started;

char log_error_prefix[32] = "<ERROR_PREFIX>-";
int  log_error_count = 0;
int  log_prefix_timestamp = 0;
int  log_prefix_stdtype   = 0;
int  log_prefix_thread    = 0;
char log_file_name[STR_SIZE] = "";
FILE log_file;

int _log_header_printed = 0;

thread_mutex log_mutex;

void _log_error_init_mutex(char *mutex_name) {
	fprintf(stderr, "%s%03d ", log_error_prefix, 48);
	fprintf(ERRORS[48], mutex_name);
	exit(2);
}

void _log_initialize(char *error_prefix) {
	clock_gettime(0, &log_time_started);
	int i;
	for(i=0; i<sizeof(log_error_prefix)-1 && error_prefix[i]; i++) log_error_prefix[i] = error_prefix[i];
	log_error_prefix[i] = 0;
	if (_thread_mutex_init_try(&log_mutex))
		_log_error_init_mutex("log_mutex");
}

void log_set_prefix(char *value) {
	log_prefix_timestamp = 0;
	log_prefix_stdtype   = 0;
	log_prefix_thread    = 0;
	int value_len = strlen(value);
	if (value_len>0 && value[0]=='t') {
		log_prefix_timestamp = 1;
	}
	if (value_len>1 && value[1]=='s') {
		log_prefix_stdtype = 1;
	}
	if (value_len>2 && value[2]=='t') {
		log_prefix_thread = 1;
	}
}

void log_set_file(char *value) {
	if (!freopen(value, "a", stdout)) {
		log_stdout_println("");
		log_stderr_print(25, errno, value, "stdout");
		exit(2);
	}
	fseek(stdout, 0, SEEK_END);
	if (ftello(stdout)!=0) {
		fprintf(stdout, "\n");
	}
	if (!freopen(value, "a", stderr)) {
		log_stdout_println(ERRORS[25], errno, value, "stderr");
		exit(2);
	}
	if (str_copy(log_file_name, sizeof(log_file_name), value))
		exit(2);
}


int log_get_uptime() {
	time_t t;
	time(&t);
	return (int) (t-log_time_started);
}

int log_get_error_count() {
	return log_error_count;
}

void _log_print_line_prefix(int stdtype) {
	FILE *stream = STDTYPE_STREAM(stdtype);
	if (log_prefix_timestamp) {
		struct timeinfo *timeinfo;
		struct timespec ts;
		char prefix[STR_SIZE];
		clock_gettime(0, &ts);
		timeinfo = localtime(&ts.tv_sec);
		strftime(prefix,sizeof(prefix),"%Y-%m-%d %H:%M:%S",timeinfo);
		fprintf(stream, "%s.%03ld ", prefix, ts.tv_nsec/1000000L);
	}
	if (log_prefix_stdtype) {
		fprintf(stream, "%s ", stdtype==STDTYPE_OUT ? "STDOUT" : "STDERR");
	}
	if (log_prefix_thread) {
		thread *thread_current;
		fprintf(stream, THREAD_NAME_FORMAT, !thread_get_current(&thread_current) ? thread_current->name : "<NOT_FOUND>");
	}
	if (log_prefix_timestamp)
		fputc(' ', stream);
}

int _log_first_line = 1;

void _log_print_char(int stdtype, FILE *stream, char c) {
	if (c=='\n') {
		if (_log_first_line) {
			_log_first_line = 0;
		} else {
			putc(c, stream);
			_log_print_line_prefix(stdtype);
		}
		return;
	}
	putc(c, stream);
}

void _log_print_str(int stdtype, const char *str) {

	FILE *stream = STDTYPE_STREAM(stdtype);

	for(int i=0; str[i]; i++)
		_log_print_char(stdtype, stream, str[i]);

    fflush(stream);

}


void _log_printf(int stdtype, const char* format, va_list args) {

	char buffer[10*1024];
	int len = vsnprintf(buffer, sizeof(buffer), format, args);

	_log_print_str(stdtype, buffer);

	int buffer_len=strlen(buffer);
	if (len!=buffer_len) {
		FILE *stream = STDTYPE_STREAM(stdtype);
		fprintf(stream, "...[%d more]", len-buffer_len);
		fflush(stream);
	}

}

void log_stdout_println(const char* format, ...) {

	thread_mutex_lock(&log_mutex);

    va_list args;
    va_start(args, format);

    _log_print_char(STDTYPE_OUT, stdout, '\n');
    _log_printf(STDTYPE_OUT, format, args);

    va_end(args);

	thread_mutex_unlock(&log_mutex);

}

void log_stdout_printf(const char* format, ...) {

	thread_mutex_lock(&log_mutex);

    va_list args;
    va_start(args, format);

    _log_printf(STDTYPE_OUT, format, args);

    va_end(args);

	thread_mutex_unlock(&log_mutex);

}

void log_stdout_print_header(char *program_desc) {
	_log_first_line=0;
	_log_print_line_prefix(STDTYPE_OUT);
	log_stdout_printf("%s\nversion %s, %s %d bits\n", program_desc, VERSION, OS_NAME, sizeof(void*)*8);
	_log_header_printed = 1;
}

void log_stdout_print_and_exit(int result) {
	thread_mutex_lock(&log_mutex);
	_log_print_str(STDTYPE_OUT, "\n\n");
	if (!_log_header_printed) {
		_log_print_str(STDTYPE_OUT, result==0 ? "completed successfully" : result==1 ? "completed with errors" : "not completed due to errors");
		_log_print_str(STDTYPE_OUT, "\n");
	}
	if (log_file_name[0]==0)
		fprintf(stdout, "\n\n");
	exit(result);
}

void log_stderr_print(int error_code, ...) {

	fflush(stdout);

	int errors_size = sizeof(ERRORS)/sizeof(ERRORS[0]);

	if (error_code<0 || error_code>=errors_size)
		error_code=errors_size-1;

	thread_mutex_lock(&log_mutex);

	log_error_count++;

	char error_text[LOG_ERROR_TEXT_SIZE];

	snprintf(error_text, sizeof(error_text), "%s%03d ", log_error_prefix, error_code);
	int prefix_len = strlen(error_text);

    va_list args;
    va_start(args, &error_code);
	vsnprintf(error_text+prefix_len, sizeof(error_text)-prefix_len, ERRORS[error_code], args);
    va_end(args);

    _log_print_str(STDTYPE_ERR, "\n");
    _log_print_str(STDTYPE_ERR, error_text);

    thread_mutex_unlock(&log_mutex);

   	thread_set_last_erorr(error_code, error_text);

}
