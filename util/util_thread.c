#include <stdio.h>

#ifdef _WIN32

#include <processthreadsapi.h>
#include <synchapi.h>
#define  CURRENT_THREAD_ID GetCurrentThreadId()

#else

#include <pthread.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#define  CURRENT_THREAD_ID syscall(__NR_gettid)

#endif

#include "util.h"

#define THREADS_SIZE      1000
#define THREADS_MAP_MASK  0xFFFF

thread *threads;

int *threads_map;

unsigned char threads_initialized = 0;
thread_mutex  threads_mutex;

int thread_mutex_init(thread_mutex *mutex, char *mutex_name) {
	#ifdef _WIN32
		*mutex = CreateMutex(NULL, FALSE, NULL);
		if (*mutex!=NULL) return 0;
	#else
		if (!pthread_mutex_init(mutex, NULL)) return 0;
	#endif
	return log_error(48, mutex_name);
}

void thread_mutex_destroy(thread_mutex *mutex) {
	#ifdef _WIN32
		CloseHandle(*mutex);
	#else
		pthread_mutex_destroy(mutex);
	#endif
}

void thread_mutex_lock(thread_mutex *mutex) {
	#ifdef _WIN32
		if (WaitForSingleObject(*mutex, -1)) {
	#else
		if (pthread_mutex_lock(mutex)) {
	#endif
			log_error(37);
			log_exit_fatal();
		}
}

int thread_mutex_try_lock(thread_mutex *mutex) {
	#ifdef _WIN32
		if (WaitForSingleObject(*mutex, 0)) {
	#else
		if (pthread_mutex_trylock(mutex)) {
	#endif
			return -1;
		}
		return 0;
}

void thread_mutex_unlock(thread_mutex *mutex) {
	#ifdef _WIN32
		if (!ReleaseMutex(*mutex)) {
	#else
		if (pthread_mutex_unlock(mutex)) {
	#endif
			log_error(38);
			log_exit_fatal();
		}
}

void thread_initialize() {
	if (thread_mem_alloc(&threads, sizeof(thread)*THREADS_SIZE))
		log_exit_fatal();
	for(int i=0; i<THREADS_SIZE; i++)
		threads[i].used = 0;
	if (thread_mem_alloc(&threads_map, sizeof(int)*(THREADS_MAP_MASK+1)))
		log_exit_fatal();
	for(int i=0; i<=THREADS_MAP_MASK; i++)
		threads_map[0] = -1;
	if (thread_mutex_init(&threads_mutex, "threads_mutex"))
		log_exit_fatal();
	threads[0].used = 2;
	threads[0].id =	CURRENT_THREAD_ID;
	threads_map[threads[0].id&THREADS_MAP_MASK] = 0;
	threads[0].last_error_code = 0;
	threads[0].last_error_text[0] = 0;
	if(str_copy(threads[0].name, sizeof(threads[0].name), "MAIN"))
		log_exit_fatal();
	threads_initialized = 1;
}

int thread_create(void *function, char *name, tcp_socket socket_connection) {
	int thread_index;
	thread_mutex_lock(&threads_mutex);
	while(1) {
		for(thread_index=0; thread_index<THREADS_SIZE && threads[thread_index].used; thread_index++);
		if (thread_index<THREADS_SIZE) break;
		log_info("too many running threads, waiting");
		sleep(1);
	};
	threads[thread_index].used = 1;
	thread_mutex_unlock(&threads_mutex);
	if (str_copy(threads[thread_index].name, sizeof(threads[0].name), name)) {
		threads[thread_index].used = 0;
		return 1;
	}
	threads[thread_index].socket_connection = socket_connection;
	#ifdef _WIN32
		threads[thread_index].sys_id = CreateThread(NULL, 0, function, thread_index, 0, NULL);
		if (threads[thread_index].sys_id == NULL) {
	#else
		if (pthread_create(&threads[thread_index].sys_id, NULL, function, thread_index) != 0) {
	#endif
			threads[thread_index].used = 0;
			return log_error(26, name);
		}
	threads[thread_index].last_error_code = 0;
	threads[thread_index].last_error_text[0] = 0;
	threads[thread_index].used = 2;
	#ifdef TRACE
		threads[thread_index].mem_allocated = 0;
	#endif
	return 0;
}

tcp_socket thread_get_socket_connection(void *thread_args) {
	int thread_index = thread_args;
	return threads[thread_index].socket_connection;
}

void thread_begin(void *thread_args) {
	int thread_index = thread_args;
	threads[thread_index].id = CURRENT_THREAD_ID;
	threads_map[threads[thread_index].id&THREADS_MAP_MASK] = thread_index;
	log_info("thread started, thread_id: %u", threads[thread_index].id);
}

void thread_end(void *thread_args) {
	int thread_index = thread_args;
	log_info("thread \"%s\" finished", threads[thread_index].name);
	threads[thread_index].used = 3;
	threads_map[threads[thread_index].id&THREADS_MAP_MASK] = -1;
	threads[thread_index].id = 0;
	threads[thread_index].used = 0;
}

int thread_get_current(thread **thread_current) {
	*thread_current = NULL;
	if (!threads_initialized) return -1;
	unsigned int thread_id = CURRENT_THREAD_ID;
	int thread_index = threads_map[thread_id&THREADS_MAP_MASK];
	if (0<=thread_index && thread_index<THREADS_SIZE && threads[thread_index].used==2 && threads[thread_index].id==thread_id) {
		*thread_current = &threads[thread_index];
		return 0;
	}
	for(int i=0; i<THREADS_SIZE; i++)
		if (threads[i].used==2 && threads[i].id==thread_id) {
			*thread_current = &threads[i];
			return 0;
		}
	return -1;
}

int thread_set_last_erorr(int error_code, char *error_text) {
	if (!threads_initialized) return 1;
	if (error_code==LOG_ERROR_NOT_FOUND_CURRENT_THREAD_CODE) return 1;
	thread *thread_current;
	if(thread_get_current(&thread_current))
		return log_error(LOG_ERROR_NOT_FOUND_CURRENT_THREAD_CODE);
	thread_current->last_error_code = error_code;
	if(str_copy_more(thread_current->last_error_text, LOG_TEXT_SIZE, error_text)) return 1;
	return 0;
}

int thread_get_last_error(int *error_code, char *error_text, int error_text_size) {
	if (!threads_initialized) return 1;
	thread *thread_current;
	if(thread_get_current(&thread_current)) {
		log_error(*error_code = LOG_ERROR_NOT_FOUND_CURRENT_THREAD_CODE);
		if(str_copy(error_text, error_text_size, LOG_ERROR_NOT_FOUND_CURRENT_THREAD_TEXT)) return 1;
		return 1;
	}
	*error_code = thread_current->last_error_code;
	if(str_copy(error_text, error_text_size, thread_current->last_error_text)) return 1;
	return 0;
}

int thread_get_count() {
	int count=0;
	for(int i=0; i<THREADS_SIZE; i++)
		if (threads[i].used==2) count++;
	return count;
}

int thread_mem_alloc(void **pointer, size_t size) {
	*pointer = malloc(size);
	if (*pointer==NULL) return log_error(9, size);
	#ifdef TRACE
		thread_allocated_change(+1);
		log_trace("%p", *pointer);
	#endif
	return 0;
}

void thread_mem_free(void **pointer) {
	if (*pointer==NULL)	{ log_error(51); return; }
	free(*pointer);
	#ifdef TRACE
		thread_allocated_change(-1);
		log_trace("%p", *pointer);
	#endif
	*pointer = NULL;
}

#ifdef TRACE

void thread_allocated_change(int mem_allocated_delta) {
	thread *thread_current;
	if (!threads_initialized || thread_get_current(&thread_current)) return;
	thread_current->mem_allocated += mem_allocated_delta;
}

#endif

void thread_mem_check_leak() {
	#ifdef TRACE
		thread *thread_current;
		if (threads_initialized && !thread_get_current(&thread_current) && thread_current->mem_allocated)
			log_error(4);
	#endif
}

#ifndef _WIN32
int thread_unix_command_execute(char *command, char *output, int output_size, int log_sucess) {
	if (output==NULL) {
		char output_tmp[STR_SIZE];
		output = output_tmp; output_size = sizeof(output_tmp);
	}
	output[0]=0;
	if (log_sucess) log_info("executing command:\n%s", command);
	char commands[STR_SIZE];
	char *argv[20];
	int argc=0;
	argv[argc] = &commands[0];
	for(int i=0;;) {
		if (command[i]!='"')
			for(;command[i] && command[i]!=' ';i++) commands[i] = command[i];
		else {
			commands[i] = command[i];
			for(i++;command[i] && !((i<2 || command[i-2]!='\\') && command[i-1]=='"' && command[i]==' ');i++) commands[i] = command[i];
		}
		commands[i]=0;
		if (!command[i]) break;
		if (argc==sizeof(argv)/sizeof(argv[0]-2)) return log_error(89);
		argv[++argc] = &commands[++i];
	}
	argv[++argc] = NULL;
	for(int i=0;i<argc;i++) {
		if (argv[i][0]!='"') continue;
		if (str_delete_char(argv[i], 0)) return 1;
		for(int p=0; argv[i][p]; p++) {
			if (argv[i][p]=='"' && !argv[i][p+1]) { argv[i][p]=0; break; }
			if (argv[i][p]=='\\' && str_delete_char(argv[i], p)) return 1;
		}
	}
    int fork_pipe[2] = {0,0};
    if (pipe(fork_pipe)) {
    	log_error(91, errno);
    	goto error;
	}
	int fork_pid=fork();
	if (fork_pid==-1) {
		log_error(88, errno);
		goto error;
	}
    if (fork_pid==0) {
  	    if (dup2(fork_pipe[1], 1)==-1 || dup2(fork_pipe[1], 2)==-1) {
  	    	fprintf(stderr, "Error duplicate fork pipe (errno %d)\n", errno);
  	    	exit(2);
  	    }
  	    exit(execvp(argv[0], argv));
    }
    int fork_status;
    if (waitpid(fork_pid, &fork_status, 0)!=fork_pid) {
    	log_error(90, errno);
    	goto error;
	}
    int fork_errno = WEXITSTATUS(fork_status);
    if (close(fork_pipe[1])) {
    	log_error(92, errno);
    	goto error;
    }
    fork_pipe[1] = 0;
    int output_len=0;
    for(int read_len; (read_len=read(fork_pipe[0], output+output_len, output_size-output_len-1))!=0; output_len+=read_len);
	output[output_len]=0;
    if (close(fork_pipe[0])) {
    	log_error(92, errno);
    	goto error;
    }
    fork_pipe[0] = 0;
    if (fork_errno) {
		char warn_command[STR_SIZE] = "";
		if(!log_sucess && str_format(warn_command, sizeof(warn_command), "command:\n%s\n", command)) return 1;
   		log_warn(905, fork_errno, warn_command, output);
   		return 2;
    } else {
    	if (log_sucess)
    		log_info("output:\n%s\ncommand executed successfully", output);
    	return 0;
    }
error:
	if (fork_pipe[0]) close(fork_pipe[0]);
	if (fork_pipe[1]) close(fork_pipe[1]);
	return 1;
}

#endif
