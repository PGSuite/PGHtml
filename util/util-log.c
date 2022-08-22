#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/time.h>

#include "util-str.h"
#include "util-file.h"
#include "util-version.h"

const char ERRORS[][100] = {
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
	"Output file too large (%d bytes)",                           // 15
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
	""                                                            //
};

char program_name[STR_SIZE_MAX]         = "<program_name>";
char program_error_prefix[STR_SIZE_MAX] = "error_prefix";

char log_info_type = 'p';
int  log_prefix_timestamp = 0;
int  log_prefix_stdtype   = 0;
int  log_prefix_pthread   = 0;
FILE log_file;

pthread_mutex_t log_mutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_t       log_pthread_main;

int log_set_program_info(char *name, char *error_prefix) {
	if (str_copy(program_name, sizeof(program_name), name)) return 1;
	if (str_copy(program_error_prefix, sizeof(program_error_prefix), error_prefix)) return 1;
	return 0;
}

void log_set_info_type (char value) { char output_prefix    = value; }
void log_set_prefix    (char *value) {
	log_prefix_timestamp = 0;
	log_prefix_stdtype   = 0;
	log_prefix_pthread   = 0;
	int value_len = strlen(value);
	if (value_len>0 && value[0]=='t') {
		log_prefix_timestamp = 1;
	}
	if (value_len>1 && value[1]=='s') {
		log_prefix_stdtype   = 1;
	}
	if (value_len>2 && value[2]=='p') {
		log_prefix_pthread   = 1;
	}
}

void log_set_file(char *value) {
	if (!freopen(value, "a", stdout)) {
		log_stdout_printf("\n");
		log_stderr_printf(25, errno, value, "stdout");
		log_stdout_print_and_exit(2);
	}
	if (!freopen(value, "a", stderr)) {
		log_stdout_printf("\n");
		log_stdout_printf(ERRORS[25], errno, value, "stderr");
		log_stdout_print_and_exit(2);
	}
}

void _log_print_header() {
	_log_print_char('o', stdout, '\n');
	fprintf(stdout, "%s version %s, %s %d bits", program_name, VERSION, OS_NAME, sizeof(void*)*8);
	_log_print_char('o', stdout, '\n');
}

void _log_print_line_prefix(char stdtype, FILE *stream) {
	if (log_prefix_timestamp) {
		struct timeinfo *timeinfo;
		struct timespec ts;
		char prefix[STR_SIZE_MAX];
		clock_gettime(0 /*CLOCK_REALTIME*/, &ts);
		timeinfo = localtime(&ts.tv_sec);
		strftime (prefix,sizeof(prefix),"%Y-%m-%d %H:%M:%S",timeinfo);
		fprintf(stream, "%s.%03ld ", prefix, ts.tv_nsec/1000000L);
	}
	if (log_prefix_stdtype) {
		fprintf(stream, "%s ", stdtype=='o' ? "STDOUT" : "STDERR");
	}
	if (log_prefix_pthread) {
		pthread_t pthread = pthread_self();
		fprintf(stream, "%06u ", pthread);
	}
	if (log_prefix_timestamp)
		fputc(' ', stream);
}

int _log_header = 1;
int _log_first_line = 1;

void _log_print_char(char stdtype, FILE *stream, char c) {
	if (_log_header) {
		_log_header = 0;
		_log_print_header();
	}
	if (c=='\n') {
		if (_log_first_line)
			_log_first_line = 0;
		else
			putc(c, stream);
		_log_print_line_prefix(stdtype, stream);
		return;
	}
	putc(c, stream);
}

void _log_printf(char stdtype, FILE *stream, const char* format, va_list args)
{
    int fmt_len = strlen(format);
    char s_out[STR_SIZE_MAX];
    int d;
    char *s;

    for(int i=0; i<fmt_len; i++) {
    	if (format[i]!='%' || i==fmt_len-1) {
    		_log_print_char(stdtype, stream, format[i]);
            continue;
    	}
    	i++;
    	switch(format[i]) {
    	case 'd':
        	d = va_arg(args, int);
        	fprintf(stream, "%d", d);
        	break;
    	case 's':
            s = va_arg(args, char*);
            snprintf(s_out, sizeof(s_out), "%s", s);
            int s_out_len = strlen(s_out);
            for(int j=0; j<s_out_len; j++)
            	_log_print_char(stdtype, stream, s_out[j]);
        	break;
        default:
        	fprintf(stream, "<not supported>");
    	}
    }

    fflush(stream);
}

void log_stdout_printf(const char* format, ...)
{

	pthread_mutex_lock(&log_mutex);

    va_list args;
    va_start(args, format);

    _log_printf('o', stdout, format, args);

    va_end(args);

	pthread_mutex_unlock(&log_mutex);

}

void log_stderr_printf(int error_code, ...)
{
	if (error_code<0 || error_code>=sizeof(ERRORS)/sizeof(ERRORS[0]))
		error_code=0;

	pthread_mutex_lock(&log_mutex);

	_log_print_char('e', stderr, '\n');
	fprintf(stderr, "%s%03d ", program_error_prefix, error_code);

    va_list args;
    va_start(args, &error_code);

    _log_printf('e', stderr, ERRORS[error_code], args);

    va_end(args);

    pthread_mutex_unlock(&log_mutex);
}

void log_stdout_print_and_exit(int result) {
	pthread_mutex_lock(&log_mutex);
	_log_printf('o', stdout, "\n\n", NULL);
	_log_printf('o', stdout, result==0 ? "completed successfully" : result==1 ? "completed with errors" : "not completed due to errors", NULL);
	_log_printf('o', stdout, "\n", NULL);
    fprintf(stdout, "\n");
	exit(result);
}
