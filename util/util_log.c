#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "util_file.h"
#include "util_str.h"
#include "util_thread.h"
#include "util_version.h"

#define STDTYPE_OUT             1
#define STDTYPE_ERR             2
#define STDTYPE_STREAM(stdtype) (stdtype==STDTYPE_OUT ? stdout : stderr)

const char *ERRORS[] = {
	"Unrecognized error",                                         //  0
	"Database сonnection error:\n%s",                             //  1
	"No value for option \"%s\"",                                 //  2
	"Non-existent option \"%s\"",                                 //  3
	"Not specified directory for processing",                     //  4
	"Destination string too small (%d bytes, %d required)",       //  5
	"Too many attributes for tag \"%s\"",                         //  6
	"Not parse attribute name from position %d of tag \"%s\"",    //  7
	"Error open file \"%s\"\n",                                   //  8
	"Can not allocate memory (%d bytes)",                         //  9
	"Error read file \"%s\"",                                     // 10
	"File \"%s\" read partially",                                 // 11
	"Error write file \"%s\"",                                    // 12
	"File \"%s\" written partially",                              // 13
	"Error close file \"%s\"",                                    // 14
	"<deleted>",               // 15
	"Not found \"%s\" for position %d",                           // 16
	"Not found char \"}\" for start position %d",                 // 17
	"Not found tag \"%s\" for start position %d",                 // 18
	"SQL error:\n%s\n%s",                                         // 19
	"List size (%d) too small (value=\"%s\")",                    // 20
	"Collection value size (%d) too small for string \"%s...\"",  // 21
	"File extension \"%s\" must start with \"pg\"",               // 22
	"Too many directories or files (max %d)",                     // 23
	"Map size (%d) too small (key=\"%s\",value=\"%s\")",          // 24
	"Error (errno %d) open log file \"%s\" for %s",               // 25
	"Can not create thread \"%s\"",                               // 26
	"Can not start WSA (errno %d)",                               // 27
	"Can not create socket (errno %d)",                           // 28
	"Can not bind socket to port %d (errno %d)",                  // 29
	"Can not listen for incoming connections (errno %d)",         // 30
	"Can not accept connection (errno %d)",                       // 31
	"Can not set timeout (errno %d)",                             // 32
	"No data to recieve from socket (timeout %d sec)",            // 33
	"Can not send to socket (errno %d)",                          // 34
	"Can not close socket (errno %d)",                            // 35
	"Can not initialize mutex",                                   // 36
	"Can not lock mutex",                                         // 37
	"Can not unlock mutex",                                       // 38
	"Incorrect administration command \"%s\"",                    // 39
	"No value, SQL query:\n%s",                                   // 40
	"Not option \"%s\"",                                          // 41
	"Incorrect port number \"%s\"",                               // 42
	"Can not connect to %s:%d (errno %d)",                        // 43
	"Process not stopped (administration socket not closed)",     // 44
	"Error on recieved data from socket (errno %d)",              // 45
	"Too many (%d) program arguments",                            // 46
	"Can not create process (errno %d), command:\n%s",            // 47
	"Can not initialize log mutex",                               // 48
	"Can not create directory \"%s\" (errno %d)",                 // 49
	"Can not get stat for path \"%s\" (errno %d)",                // 50
	"File body is null (memory not allocated)",                   // 51
	"Unrecognized command (\"%s\")",                              // 52
	"<deleted>",                 // 53
	"Incorrect parameters (%s)",                                  // 54
	""                                                            //
};

char log_error_prefix[STR_SIZE_MAX] = "<error_prefix>";
int  log_error_count = 0;
int  log_prefix_timestamp = 0;
int  log_prefix_stdtype   = 0;
int  log_prefix_thread    = 0;
char log_file_name[STR_SIZE_MAX] = "";
FILE log_file;

int  _log_header_printed = 0;

THREAD_MUTEX_T log_mutex;

void log_initialize(char *error_prefix) {
	if (thread_mutex_init_try(&log_mutex)) {
		fprintf(stderr, "%s%03d %s\n", error_prefix, 48, ERRORS[48]);
		exit(2);
	}
	if (str_copy(log_error_prefix, sizeof(log_error_prefix), error_prefix))
		exit(2);
}

void log_set_prefix (char *value) {
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

int log_get_error_count() {
	return log_error_count;
}

void _log_print_line_prefix(int stdtype) {
	FILE *stream = STDTYPE_STREAM(stdtype);
	if (log_prefix_timestamp) {
		struct timeinfo *timeinfo;
		struct timespec ts;
		char prefix[STR_SIZE_MAX];
		clock_gettime(0, &ts);
		timeinfo = localtime(&ts.tv_sec);
		strftime(prefix,sizeof(prefix),"%Y-%m-%d %H:%M:%S",timeinfo);
		fprintf(stream, "%s.%03ld ", prefix, ts.tv_nsec/1000000L);
	}
	if (log_prefix_stdtype) {
		fprintf(stream, "%s ", stdtype==STDTYPE_OUT ? "STDOUT" : "STDERR");
	}
	if (log_prefix_thread) {
		thread_print_name(stream);
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

void _log_printf(int stdtype, const char* format, va_list args) {

	FILE *stream = STDTYPE_STREAM(stdtype);

	char buffer[10*1024];
	int len = vsnprintf(buffer, sizeof(buffer), format, args);

	int i=0;
	for(; buffer[i]; i++) {
		_log_print_char(stdtype, stream, buffer[i]);
	}

	if (i!=len)
		fprintf(stream, "...[%d more]", len-i);

    fflush(stream);
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

void log_stderr_print(int error_code, ...) {

	if (error_code<0 || error_code>=sizeof(ERRORS)/sizeof(ERRORS[0]))
		error_code=0;

	thread_mutex_lock(&log_mutex);

	log_error_count++;

	fflush(stdout);

	_log_print_char(STDTYPE_ERR, stderr, '\n');
	fprintf(stderr, "%s%03d ", log_error_prefix, error_code);

    va_list args;
    va_start(args, &error_code);

    _log_printf(STDTYPE_ERR, ERRORS[error_code], args);

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
	_log_printf(STDTYPE_OUT, "\n\n", NULL);
	if (!_log_header_printed) {
		_log_printf(STDTYPE_OUT, result==0 ? "completed successfully" : result==1 ? "completed with errors" : "not completed due to errors", NULL);
		_log_printf(STDTYPE_OUT, "\n", NULL);
	}
	if (log_file_name[0]==0)
		fprintf(stdout, "\n\n");
	exit(result);
}
