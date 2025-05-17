#include <stdio.h>
#include <semaphore.h>

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

thread_mutex_t thread_create_mutex;
sem_t          thread_create_sem;
int            thread_name_len_max;
unsigned char  threads_initialized = 0;

__thread thread_info_t thread_info;

void _thread_mutex_init(thread_mutex_t *mutex, char *p_mutex_name) {
	#ifdef _WIN32
		*mutex = CreateMutex(NULL, FALSE, NULL);
		if (*mutex!=NULL) return;
	#else
		if (!pthread_mutex_init(mutex, NULL)) return;
	#endif
	log_error(1048, p_mutex_name);
	log_exit_fatal();
}

void thread_mutex_destroy(thread_mutex_t *mutex) {
	#ifdef _WIN32
		CloseHandle(*mutex);
	#else
		pthread_mutex_destroy(mutex);
	#endif
}

void thread_mutex_lock(thread_mutex_t *mutex) {
	#ifdef _WIN32
		if (WaitForSingleObject(*mutex, -1)) {
	#else
		if (pthread_mutex_lock(mutex)) {
	#endif
			log_error(1037);
			log_exit_fatal();
		}
}

int thread_mutex_try_lock(thread_mutex_t *mutex) {
	#ifdef _WIN32
		if (WaitForSingleObject(*mutex, 0)) {
	#else
		if (pthread_mutex_trylock(mutex)) {
	#endif
			return -1;
		}
		return 0;
}

void thread_mutex_unlock(thread_mutex_t *mutex) {
	#ifdef _WIN32
		if (!ReleaseMutex(*mutex)) {
	#else
		if (pthread_mutex_unlock(mutex)) {
	#endif
			log_error(1038);
			log_exit_fatal();
		}
}

void _thread_semaphore_init(sem_t *sem, char *p_sem_name) {
	if (sem_init(sem, 0, 0)) {
		log_error(1095, p_sem_name);
		log_exit_fatal();
	}
}

void thread_semaphore_wait(sem_t *sem) {
	if (sem_wait(sem)) {
		log_error(1096);
		log_exit_fatal();
	}
}

void thread_semaphore_post(sem_t *sem) {
	if (sem_post(sem)) {
		log_error(1097);
		log_exit_fatal();
	}
}

void thread_initialize(int name_len_max) {
	thread_mutex_init(&thread_create_mutex);
	thread_semaphore_init(&thread_create_sem);
	if(str_copy(thread_info.name, sizeof(thread_info.name), "MAIN"))
		log_exit_fatal();
	#ifdef TRACE
		thread_info.mem_allocated = 0;
	#endif
	thread_name_len_max = name_len_max;
	threads_initialized = 1;
}

int thread_create(void *function(thread_params_t *params), thread_params_t *params) {
	thread_mutex_lock(&thread_create_mutex);
	int error;
	#ifdef _WIN32
		if (CreateThread(NULL, 0, function, params, 0, NULL)==NULL) {
			error = GetLastError();
	#else
		pthread_t th;
		if (error=pthread_create(&th, NULL, function, params)) {
	#endif
		thread_mutex_unlock(&thread_create_mutex);
		return log_error(1026, error);
	}
	thread_semaphore_wait(&thread_create_sem);
	thread_mutex_unlock(&thread_create_mutex);
	return 0;
}

void thread_begin(char *name) {
	str_copy_more(thread_info.name, sizeof(thread_info.name), name);
	thread_semaphore_post(&thread_create_sem);
	log_info("thread started, thread_id: %u", CURRENT_THREAD_ID);
}

void thread_end() {
	log_info("thread \"%s\" finished", thread_info.name);
}

int thread_add_name(char *dest, int dest_size) {
	if (!threads_initialized) return 0;
	return str_add_format(dest, dest_size, "%-*s", thread_name_len_max, thread_info.name);
}

int thread_mem_alloc(void **pointer, size_t size) {
	*pointer = malloc(size);
	if (*pointer==NULL) return log_error(1009, size);
	#ifdef TRACE
		thread_allocated_change(+1);
		log_trace("%p", *pointer);
	#endif
	return 0;
}

void thread_mem_free(void **pointer) {
	if (*pointer==NULL)	{ log_error(1051); return; }
	free(*pointer);
	#ifdef TRACE
		thread_allocated_change(-1);
		log_trace("%p", *pointer);
	#endif
	*pointer = NULL;
}

#ifdef TRACE

void thread_allocated_change(int mem_allocated_delta) {
	thread_info.mem_allocated += mem_allocated_delta;
}

void thread_mem_check_leak() {
	if (thread_info.mem_allocated)
		log_error(1004);
}

int thread_mem_allocated_get() {
	return thread_info.mem_allocated;
}

#else

#define thread_mem_check_leak()

#endif


#ifndef _WIN32
int thread_unix_command_execute(char *command, char *output, int output_size, int log_sucess) {
	char output_tmp[STR_SIZE];
	if (output==NULL) {
		output = output_tmp; output_size = sizeof(output_tmp);
	}
	output[0]=0;
	if (log_sucess) log_info("executing command:\n%s", command);
	char commands[STR_SIZE];
	if (str_copy(commands, sizeof(commands), command)) return 1;
	char *argv[20];
	int argc=0;
	argv[argc] = &commands[0];
	for(int i=0;;) {
		if (command[i]!='"')
			for(;command[i] && command[i]!=' ';i++);
		else
			for(i++;command[i] && !((i<2 || command[i-2]!='\\') && command[i-1]=='"' && command[i]==' ');i++);
		commands[i]=0;
		if (!command[i]) break;
		if (argc==sizeof(argv)/sizeof(argv[0]-2)) return log_error(1089);
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
    	log_error_errno(91, errno);
    	goto error;
	}
	int fork_pid=fork();
	if (fork_pid==-1) {
		log_error_errno(1088, errno);
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
    	log_error_errno(1090, errno);
    	goto error;
	}
    int fork_errno = WEXITSTATUS(fork_status);
    if (close(fork_pipe[1])) {
    	log_error_errno(1092, errno);
    	goto error;
    }
    fork_pipe[1] = 0;
    int output_len=0;
    for(int read_len; (read_len=read(fork_pipe[0], output+output_len, output_size-output_len-1))!=0; output_len+=read_len);
	output[output_len]=0;
    if (close(fork_pipe[0])) {
    	log_error_errno(1092, errno);
    	goto error;
    }
    fork_pipe[0] = 0;
    if (fork_errno) {
		char warn_command[STR_SIZE] = "";
		if(!log_sucess && str_format(warn_command, sizeof(warn_command), "command:\n%s\n", command)) return 1;
   		log_warn_errno(9005, fork_errno, warn_command, output);
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
