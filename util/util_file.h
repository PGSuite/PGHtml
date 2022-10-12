#ifndef UTIL_FILE_H_
#define UTIL_FILE_H_

#ifdef _WIN32
#define FILE_SEPARATOR  "\\"
#define PATH_SEPARATOR  ";"
#define OS_NAME         "windows"
#else
#define FILE_SEPARATOR  "/"
#define PATH_SEPARATOR  ":"
#define OS_NAME         "linux"
#endif

typedef struct
{
	int	 size;
	int	 size_max;
	char *body;
} FILE_BODY;

#endif /* UTIL_FILE_H_ */
