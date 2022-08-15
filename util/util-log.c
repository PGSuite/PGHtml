#include <stdarg.h>
#include <stdio.h>
#include <time.h>
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
	""                                                            //
};

char program_name[STR_SIZE_MAX] = "<program_name>";
char error_prefix[STR_SIZE_MAX] = "error_prefix";
char log_info_type = 'p';
char log_prefix    = 'n';

int log_set_program_name(char *value) { return  (str_copy(program_name, sizeof(program_name), value)); }
int log_set_error_prefix(char *value) { return  (str_copy(error_prefix, sizeof(error_prefix), value)); }

void log_set_info_type (char value) { char output_prefix    = value; }
void log_set_prefix    (char value) { char output_info_type = value; }

void _log_print_header() {
	log_stdout_printf('p', "\n%s version %s, %s %d bits", program_name, VERSION, OS_NAME, sizeof(void*)*8);
	log_stdout_printf('p', "\n");
}

void _log_print_line_prefix(FILE *stream) {
	if (log_prefix!='t') return;
	struct timeinfo *timeinfo;
	struct timespec ts;
	char prefix[STR_SIZE_MAX];
	clock_gettime(0 /*CLOCK_REALTIME*/, &ts);
	timeinfo = localtime(&ts.tv_sec);
	strftime (prefix,sizeof(prefix),"%Y-%m-%d %H:%M:%S",timeinfo);
	fprintf(stream, "%s.%03ld  ", prefix, ts.tv_nsec/1000000L);
}

int _log_header = 1;
int _log_first_line = 1;

void _log_print_char(FILE *stream, char c) {
	if (_log_header) {
		_log_header = 0;
		_log_print_header();
	}
	if (c=='\n') {
		if (_log_first_line)
			_log_first_line = 0;
		else
			putc(c, stream);
		_log_print_line_prefix(stream);
		return;
	}
	putc(c, stream);
}

void _log_printf(FILE *stream, const char* format, va_list args)
{
    int fmt_len = strlen(format);
    char s_out[STR_SIZE_MAX];
    int d;
    char *s;

    for(int i=0; i<fmt_len; i++) {
    	if (format[i]!='%' || i==fmt_len-1) {
    		_log_print_char(stream, format[i]);
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
            	_log_print_char(stream, s_out[j]);
        	break;
        default:
        	fprintf(stream, "<not supported>");
    	}
    }

    fflush(stream);
}

void log_stdout_printf(char info_type, const char* format, ...)
{

	if (log_info_type!=info_type) return;

    va_list args;
    va_start(args, format);

    _log_printf(stdout, format, args);

    va_end(args);

}

void log_stderr_printf(int error_code, ...)
{
	if (error_code<0 || error_code>=sizeof(ERRORS)/sizeof(ERRORS[0]))
		error_code=0;

	fflush(stdout); // synchronize stdout and stderr
	usleep(10*1000);

	_log_print_char(stderr, '\n');
	fprintf(stderr, "%s%03d ", error_prefix, error_code);

    va_list args;
    va_start(args, &error_code);

    _log_printf(stderr, ERRORS[error_code], args);

    va_end(args);

    usleep(10*1000);
}

void log_stdout_print_and_exit() {
	log_stdout_printf('p', "\n");
	log_stdout_printf('p', "\nnot completed due to errors");
	log_stdout_printf('p', "\n");
	exit(2);
}
