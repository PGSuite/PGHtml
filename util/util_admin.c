#include <stdio.h>
#include <sys/types.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include "util.h"

int                      admin_port;
char                     *(*admin_commands_name)[];
admin_command_function_t *(*admin_commands_function)[];

void* _admin_thread(thread_params_t *params) {
	thread_begin("ADMINISTRATION");
    tcp_socket socket_listen;
	if (tcp_unix_socket_create(&socket_listen)) log_exit_fatal();
	if (tcp_unix_bind(socket_listen, admin_port)) log_exit_fatal();
	if (tcp_socket_listen(socket_listen)) log_exit_fatal();
	log_info("listening");
	while (1) {
		tcp_socket socket_connection;
		if (tcp_socket_accept(socket_listen, &socket_connection)) continue;
		log_info("connection accepted");
		tcp_set_socket_timeout(socket_connection);
		char command[STR_SIZE];
		if (tcp_recv_str(socket_connection, command, sizeof(command))) {
			tcp_socket_close(socket_connection);
			continue;
		}
		log_info("received command \"%s\"", command);
		if (!strcmp(command, "stop")) {
			log_exit_stop();
		}
		for(int i=0;;i++) {
			if ((*admin_commands_name)[i]==NULL) {
				log_error(39, command);
				break;
			}
			if (!strcmp(command,(*admin_commands_name)[i])) {
				char info[ADMIN_INFO_SIZE] = "";
				if ((*admin_commands_function)[i](info, sizeof(info))) {
					str_copy(info, sizeof(info), "server error, see log");
				}
				if (!tcp_send(socket_connection, info, strlen(info)+1))
					log_info("info sent");
				break;
			}
		}
		tcp_socket_close(socket_connection);
	}
	thread_end();
	return 0;
}

int admin_thread_create() {
	return thread_create(_admin_thread, NULL);
}

void admin_check_command(int argc, char *argv[], int application_port, const char *commands_name[], const admin_command_function_t *commands_function[])
{

	if (!strcmp(argv[1],"start")) {
		argv[1] = "execute";
		int status;
		int error_code;
		unsigned int pid;
		char command[4*1024] = "";
		for(int i=0; i<argc; i++)
			if (str_add(command, sizeof(command), i>0 ? " " : "", argv[i], NULL)) exit(3);
		#ifdef _WIN32
			STARTUPINFO cif;
			ZeroMemory(&cif,sizeof(STARTUPINFO));
			PROCESS_INFORMATION pi;
			status = !CreateProcess(argv[0], command, NULL,NULL,FALSE,NULL,NULL,NULL,&cif,&pi);
			error_code =  GetLastError();
			pid = pi.hProcess;
		#else
			status = posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL);
		#endif
		if (status) {
			log_error(36, error_code, command);
			exit(3);
		}
		exit(0);
	}

	#ifdef _WIN32
		admin_port = application_port+10000;
	#else
		admin_port = application_port;
	#endif
	admin_commands_name     = commands_name;
	admin_commands_function = commands_function;

	char command[STR_SIZE] = "";
	for(int i=1; i<argc; i++ )
		if (str_add(command, sizeof(command), i>1 ? " " : "", argv[i], NULL)) log_exit_fatal();

	if (strcmp(command,"stop"))
		for(int i=0;;i++) {
			if ((*admin_commands_name)[i]==NULL) return;
			if (!strcmp(command,(*admin_commands_name)[i])) break;
		}

	if (tcp_startup()) exit(2);

	tcp_socket sock;
	if (tcp_unix_socket_create(&sock)) exit(2);

    if(tcp_unix_connect(sock, admin_port)) exit(2);

    tcp_set_socket_timeout(sock);

    if (tcp_send(sock, command, strlen(command)+1)) exit(2);

    if (!strcmp(argv[1],"stop")) {
    	printf("stopping...");
		char c = '1';
		for (int i=0; i<10; i++) {
			if (send(sock, &c, 1, TCP_SEND_FLAGS)<0) {
				printf(" done\n");
				exit(0);
			}
			sleep(1);
			printf(".");
			fflush(stdout);
		}
		printf("\n");
		log_error(44);
		exit(2);
    }

   	char info[ADMIN_INFO_SIZE];
   	int res = tcp_recv_str(sock, info, sizeof(info));
   	if (!res)
   		printf("%s\n", info);
   	exit(res);

}
