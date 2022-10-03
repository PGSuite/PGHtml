#include <stdio.h>

#ifdef _WIN32

#include <processthreadsapi.h>
#include <synchapi.h>
#define  CURRENT_THREAD_ID GetCurrentThreadId()

#else

#include <pthread.h>
#define  CURRENT_THREAD_ID pthread_self()

#endif

#include "util_thread.h"

#define THREADS_SIZE        1000
#define THREAD_PRINT_FORMAT "%-11s "

THREAD_T threads[THREADS_SIZE];

THREAD_MUTEX_T threads_mutex;

int thread_mutex_init_try(THREAD_MUTEX_T *mutex) {
	#ifdef _WIN32
		*mutex = CreateMutex(NULL, FALSE, NULL);
		return *mutex==NULL;
	#else
		return pthread_mutex_init(mutex, NULL);
	#endif
}

void thread_mutex_init(THREAD_MUTEX_T *mutex) {
	if (thread_mutex_init_try(mutex)) {
		log_stderr_print(36);
		log_stdout_print_and_exit(2);
	}
}

void thread_mutex_lock(THREAD_MUTEX_T *mutex) {
	#ifdef _WIN32
		if (WaitForSingleObject(*mutex, -1)) {
	#else
		if (pthread_mutex_lock(mutex)) {
	#endif
			log_stderr_print(37);
			log_stdout_print_and_exit(2);
		}
}

void thread_mutex_unlock(THREAD_MUTEX_T *mutex) {
	#ifdef _WIN32
		if (!ReleaseMutex(*mutex)) {
	#else
		if (pthread_mutex_unlock(mutex)) {
	#endif
			log_stderr_print(38);
			log_stdout_print_and_exit(2);
		}
}


void thread_initialize() {
	thread_mutex_init(&threads_mutex);
	threads[0].used = 1;
	threads[0].id =	CURRENT_THREAD_ID;
	str_copy(threads[0].name, sizeof(threads[0].name), "MAIN");
	for(int i=1; i<THREADS_SIZE; i++)
		threads[i].used = 0;
}

int thread_create(void *function, char *name, TCP_SOCKET_T socket_connection) {

	int thread_index;

	thread_mutex_lock(&threads_mutex);
	while(1) {
		for(thread_index=0; thread_index<THREADS_SIZE && threads[thread_index].used; thread_index++);
		if (thread_index<THREADS_SIZE) break;
		log_stdout_println("too many running threads, waiting");
		sleep(1);
	};
	thread_mutex_unlock(&threads_mutex);

	threads[thread_index].used = 1;
	if (str_copy(threads[thread_index].name, sizeof(threads[0].name), name)) {
		threads[thread_index].used = 0;
		return 1;
	}
	threads[thread_index].socket_connection = socket_connection;
	#ifdef _WIN32
		THREAD_SYS_T thread_sys = CreateThread(NULL, 0, function, thread_index, 0, NULL);
		if (thread_sys == NULL) {
	#else
		THREAD_SYS_T thread_sys;
		if (pthread_create(&thread_sys, NULL, function, thread_index) != 0) {
	#endif
			threads[thread_index].used = 0;
			log_stderr_print(26, name);
			return 1;
		}
	threads[thread_index].thread_sys = thread_sys;
	return 0;

}

TCP_SOCKET_T thread_get_socket_connection(void *thread_args) {

	int thread_index = thread_args;
	return threads[thread_index].socket_connection;

}


void thread_begin(void *thread_args) {

	int thread_index = thread_args;

	threads[thread_index].id =	CURRENT_THREAD_ID;

	log_stdout_println("thread \"%s\" started", threads[thread_index].name);

}


void thread_end(void *thread_args) {

	int thread_index = thread_args;

	log_stdout_println("thread \"%s\" finished", threads[thread_index].name);

	threads[thread_index].id = 0;
	threads[thread_index].used = 0;

}

void thread_print_name(FILE *stream) {
	unsigned int current_id = CURRENT_THREAD_ID;
	for(int i=0; i<THREADS_SIZE; i++)
		if (threads[i].used && threads[i].id==current_id) {
			fprintf(stream, THREAD_PRINT_FORMAT, threads[i].name);
			return;
		}
	fprintf(stream, THREAD_PRINT_FORMAT, "<NOT_FOUND>");
}

int thread_get_count() {
	int count=0;
	for(int i=0; i<THREADS_SIZE; i++)
		if (threads[i].used) count++;
	return count;
}
