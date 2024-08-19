#include <stdio.h>
#include <sys/types.h>

#include "globals.h"
#include "util/utils.h"

#define ADMIN_ADDR "127.0.0.1"

int admin_server_get_status_info(char *status_info, int status_info_size) {
	if (log_get_header(status_info, status_info_size)) return 1;
	if (globals_add_parameters(status_info, status_info_size)) return 1;
	int uptime = log_get_uptime();
	int uptime_s = uptime%60, uptime_m = (uptime/60)%60, uptime_h = (uptime/60/60)%24, uptime_d = uptime/60/60/24;
	return str_add_format(status_info, status_info_size,
		"\nStatus info"
		"\n  uptime:  %3d %02d:%02d:%02d"
		"\n  threads: %3d"
		"\n\n",
		uptime_d, uptime_h, uptime_m, uptime_s,
		thread_get_count()
	);
}

void* admin_server_thread(void *args) {

	thread_begin(args);

    tcp_socket socket_listen;

	if (tcp_socket_create(&socket_listen)) log_exit_fatal();

	if (tcp_bind(socket_listen, ADMIN_ADDR, admin_port)) log_exit_fatal();
	log_info("listening socket bound to %s:%d", ADMIN_ADDR, admin_port);

	if (tcp_socket_listen(socket_listen)) log_exit_fatal();

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
    	if (!strcmp(command, "status")) {
    		char status_info[STR_SIZE] = "";
    		if (admin_server_get_status_info(status_info, sizeof(status_info))) {
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

void admin_server_command(int argc, char *argv[]) {

	http_port = atoi(HTTP_PORT_DEFAULT);

	for(int i=2; i<argc; i++) {
		if (argv[i][0]!='-') {
			log_error(41, argv[i]);
			exit(3);
		}
		if (i==argc-1) {
			log_error(2, argv[i]);
			exit(3);
		}

		if (strcmp(argv[i],"-hp")==0) {
			http_port = atoi(argv[++i]);
			if (http_port<=0) {
				log_error(42, argv[i]);
				exit(3);
			}
		} else {
			log_error(3, argv[i]);
			exit(3);
		}
	}

	admin_port = http_port + ADMIN_PORT_OFFSET;

	if (tcp_startup()) exit(2);

	tcp_socket socket;
	if (tcp_socket_create(&socket)) exit(2);

    if(tcp_connect(socket, ADMIN_ADDR, admin_port)) exit(2);

	tcp_set_socket_timeout(socket);

	if (tcp_send(socket, argv[1], strlen(argv[1])+1)) exit(2);

    if (!strcmp(argv[1],"stop")) {
    	printf("stopping...");
		char c = '1';
		for (int i=0; i<10; i++) {
			if (send(socket, &c, 1, 0)<0) {
				printf(" done");
				exit(2);
			}
			sleep(1);
			printf(".");
		}
		log_error(44);
		exit(2);
    }

    if (!strcmp(argv[1],"status")) {
    	char status_info[STR_SIZE];
    	if (!tcp_recv_str(socket, status_info, sizeof(status_info))) {
    		printf("%s\n", status_info);
    	}
        exit(0);
    }

}
