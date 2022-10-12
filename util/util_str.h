#ifndef UTIL_STR_H_
#define UTIL_STR_H_

#include <string.h>

#define STR_SIZE_MAX 1024

#define STR_COLLECTION_SIZE_MAX        30
#define STR_COLLECTION_KEY_SIZE_MAX    50
#define STR_COLLECTION_VALUE_SIZE_MAX 300

typedef struct
{
	int	 size;
	char keys  [STR_COLLECTION_SIZE_MAX][STR_COLLECTION_KEY_SIZE_MAX];
	char values[STR_COLLECTION_SIZE_MAX][STR_COLLECTION_VALUE_SIZE_MAX];
} STR_MAP;


typedef struct
{
	int	 size;
	char values[STR_COLLECTION_SIZE_MAX][STR_COLLECTION_VALUE_SIZE_MAX];
} STR_LIST;

#endif /* UTIL_STR_H_ */
