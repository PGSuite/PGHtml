#ifndef UTIL_STR_H_
#define UTIL_STR_H_

#ifndef NULL
#define NULL ((void *)0)
#endif

#define STR_SIZE_MAX 1024

#define ARRAY_SIZE_MAX         30
#define ARRAY_ELEMENT_SIZE_MAX 200

typedef struct
{
	int	 size;
	char names [ARRAY_SIZE_MAX][ARRAY_ELEMENT_SIZE_MAX];
	char values[ARRAY_SIZE_MAX][ARRAY_ELEMENT_SIZE_MAX];
} VARS;

#endif /* UTIL_STR_H_ */
