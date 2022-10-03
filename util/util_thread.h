#ifndef UTIL_THREAD_H_
#define UTIL_THREAD_H_

#ifdef _WIN32

#include <processthreadsapi.h>
#define THREAD_SYS_T   HANDLE
#define THREAD_MUTEX_T HANDLE

#else

#include <pthread.h>
#define THREAD_SYS_T   pthread_t
#define THREAD_MUTEX_T pthread_mutex_t

#endif

#include "util_tcp.h"

#define THREAD_NAME_SIZE_MAX 20

typedef struct
{

	int used;
	THREAD_SYS_T thread_sys;
	unsigned int id;
	char name[THREAD_NAME_SIZE_MAX];
	TCP_SOCKET_T socket_connection;

} THREAD_T;

#endif /* UTIL_THREAD_H_ */
