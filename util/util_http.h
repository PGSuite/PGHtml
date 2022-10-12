#include "util_file.h"

#ifndef UTIL_HTTP_H_
#define UTIL_HTTP_H_

#define HTTP_STATUS_OK             200
#define HTTP_STATUS_BAD_REQUEST    400
#define HTTP_STATUS_NOT_FOUND      404
#define HTTP_STATUS_INTERNAL_ERROR 500

typedef struct
{
	char method[16];
	char path[2048];
	char protocol[16];

	FILE_BODY content;

} HTTP_REQUEST;

#endif /* UTIL_HTTP_H_ */
