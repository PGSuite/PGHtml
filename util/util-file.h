#ifndef UTIL_FILE_H_
#define UTIL_FILE_H_

#ifdef _WIN32
#define FILE_SEPARATOR "\\"
#define PATH_SEPARATOR ";"
#define OS_NAME        "windows"
#else
#define FILE_SEPARATOR "/"
#define PATH_SEPARATOR ":"
#define OS_NAME        "linux"
#endif

#define FILE_SIZE_MAX 10*1024*1024

typedef struct
{
	int	 size;
	char *body;
} FILE_BODY;

#endif /* UTIL_FILE_H_ */
