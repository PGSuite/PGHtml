#include <stdio.h>
#include <sys/types.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include "util.h"

int admin_port;
int (*admin_function_get_status_info)(char *status_info, int status_info_size);

void admin_initialize(int port, int (*function_get_status_info)(char *status_info, int status_info_size)) {
	#ifdef _WIN32
		admin_port = port+10000;
	#else
		admin_port = port;
	#endif
	admin_function_get_status_info = function_get_status_info;
}

void* admin_server_thread(void *args) {

	thread_begin(args);

    tcp_socket socket_listen;

	if (tcp_unix_socket_create(&socket_listen)) log_exit_fatal();

	if (tcp_unix_bind(socket_listen, admin_port)) log_exit_fatal();

	if (tcp_socket_listen(socket_listen)) log_exit_fatal();
	log_info("listening");

	while (1) {
		tcp_socket socket_connection;

		if (tcp_socket_accept(socket_listen, &socket_connection)) continue;
		log_info("connection accepted");

		// tcp_set_socket_timeout(socket_connection);

		char command[STR_SIZE];
		if (tcp_recv_str(socket_connection, command, sizeof(command))) {
			tcp_socket_close(socket_connection);
			continue;
		}
		log_info("received command \"%s\"", command);

		if (!strcmp(command, "stop")) {
			log_exit_stop();
		}

		if (!strcmp(command, "status")) {
			char status_info[STR_SIZE] = "";
			if (admin_function_get_status_info(status_info, sizeof(status_info))) {
				tcp_socket_close(socket_connection);
				continue;
			}
			if (!tcp_send(socket_connection, status_info, strlen(status_info)+1))
				log_info("status info sent");
		} else {
			log_error(52, command);
		}
		tcp_socket_close(socket_connection);
	}

	thread_end(args);
	return 0;

}

void admin_server_command(int argc, char *argv[])
{

	if (tcp_startup()) exit(2);

	tcp_socket sock;
	if (tcp_unix_socket_create(&sock)) exit(2);

    if(tcp_unix_connect(sock, admin_port)) exit(2);

    tcp_set_socket_timeout(sock);

    if (tcp_send(sock, argv[1], strlen(argv[1])+1)) exit(2);

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

    if (!strcmp(argv[1],"status")) {
    	char status_info[STR_SIZE];
    	if (!tcp_recv_str(sock, status_info, sizeof(status_info))) {
    		printf("%s\n", status_info);
    	}
        exit(0);
    }

}
