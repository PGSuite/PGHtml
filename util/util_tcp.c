#include "util_tcp.h"

#include <stdio.h>
#include <sys/types.h>

#include "util_file.h"

int tcp_startup() {

    #ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2,2), &wsa_data)) {
			log_stderr_print(27, WSAGetLastError());
			return 1;
		}
		log_stdout_println("WSA started, status is \"%s\"", wsa_data.szSystemStatus);
	#endif

	return 0;

}

int tcp_socket_create(TCP_SOCKET_T *tcp_socket) {
	*tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	#ifdef _WIN32
		if(tcp_socket == INVALID_SOCKET) {
	#else
		if(tcp_socket < 0) {
	#endif
			log_stderr_print(28, TCP_ERRNO);
			return 1;
		}
	return 0;
}


int tcp_connect(TCP_SOCKET_T socket, char *addr, int port) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_addr.s_addr = inet_addr(addr);
	sockaddr.sin_port = htons(port);
	sockaddr.sin_family = AF_INET;
    if(connect(socket, &sockaddr, sizeof(sockaddr))) {
		log_stderr_print(43, addr, port, TCP_ERRNO);
    	return 1;
    }
    return 0;
}

int tcp_bind(TCP_SOCKET_T socket, char *addr, int port) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	if (addr!=NULL)
		sockaddr.sin_addr.s_addr = inet_addr(addr);
	else
		sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_family = AF_INET;
	if (bind(socket, &sockaddr, sizeof(sockaddr))) {
		log_stderr_print(29, port, TCP_ERRNO);
		return 1;
	}
	return 0;
}

int tcp_socket_listen(TCP_SOCKET_T socket) {
	if (listen(socket, 5))  {
		log_stderr_print(30, TCP_ERRNO);
		return 1;
	}
	return 0;
}

int tcp_socket_accept(TCP_SOCKET_T socket_listen, TCP_SOCKET_T *socket_connection) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	int sockaddr_size = sizeof(sockaddr);
	*socket_connection = accept(socket_listen, &sockaddr, &sockaddr_size);
	#ifdef _WIN32
		if (*socket_connection==INVALID_SOCKET) {
	#else
		if (*socket_connection<0) {
	#endif
			log_stderr_print(31, TCP_ERRNO);
			return 1;
		}
	return 0;
}

int tcp_set_socket_timeout(TCP_SOCKET_T socket) {
	#ifdef _WIN32
		int timeout = TCP_TIMEOUT*1000;
	#else
		struct timeval timeout;
		timeout.tv_sec = TCP_TIMEOUT;
		timeout.tv_usec = 0;
	#endif

	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
		setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) ) {
		log_stderr_print(32, TCP_ERRNO);
		return 1;
	}
	return 0;
}

int tcp_recv_str(TCP_SOCKET_T socket, char *str, int str_size) {
	size_t str_len=0;
	do {
		int recv_len = recv(socket, str+str_len, str_size-str_len, 0);
		if (recv_len==0
		#ifdef _WIN32
			|| (recv_len<0 && TCP_ERRNO==10060)
		#endif
		) {
			log_stderr_print(33, TCP_TIMEOUT);
			return -1;
		}
		if (recv_len<0) {
			log_stderr_print(45, TCP_ERRNO);
			return -1;
		}
		if (str_len+recv_len>str_size) {
			log_stderr_print(5, str_size, str_len+recv_len);
			return 1;
		}
		str_len += recv_len;
	} while(str[str_len-1]!=0);
	return 0;
}



int tcp_send(TCP_SOCKET_T socket, char *body, int size) {
	while (size>0) {
		int send_len = send(socket, body, size, 0);
		if (send_len<=0) {
			log_stderr_print(34, TCP_ERRNO);
			return 1;
		}
		size -= send_len;
		body += send_len;
	}
	return 0;
}

int tcp_send_str(TCP_SOCKET_T socket, char *str, int end_zero) {
	return tcp_send(socket, str, strlen(str)+(end_zero ? 1 : 0));
}

int tcp_send_file_body(TCP_SOCKET_T socket, FILE_BODY file_body) {
	return tcp_send(socket, file_body.body, file_body.size);
}


int tcp_socket_close(TCP_SOCKET_T socket) {
	#ifdef _WIN32
		if (closesocket(socket)) {
	#else
		if (close(socket)) {
	#endif
			log_stderr_print(35, TCP_ERRNO);
			return 1;
		}
	return 0;
}
