#include <stdio.h>
#include <sys/types.h>

#include "utils.h"

int tcp_startup() {
    #ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2,2), &wsa_data))
			return log_error(27, WSAGetLastError());
		// log_info("Windows Socket Architecture (WSA) started, status: \"%s\"", wsa_data.szSystemStatus);
	#endif
	return 0;
}

int tcp_socket_create(tcp_socket *sock) {
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	#ifdef _WIN32
		if(sock == INVALID_SOCKET)
	#else
		if(sock < 0)
	#endif
			return log_error(28, tcp_errno);
	return 0;
}


int tcp_connect(tcp_socket socket, char *addr, int port) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_addr.s_addr = inet_addr(addr);
	sockaddr.sin_port = htons(port);
	sockaddr.sin_family = AF_INET;
    if(connect(socket, &sockaddr, sizeof(sockaddr)))
    	return log_error(43, addr, port, tcp_errno);
    return 0;
}

int tcp_bind(tcp_socket socket, char *addr, int port) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	if (addr!=NULL)
		sockaddr.sin_addr.s_addr = inet_addr(addr);
	else
		sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_family = AF_INET;
	if (bind(socket, &sockaddr, sizeof(sockaddr)))
		return log_error(29, port, tcp_errno);
	return 0;
}

int tcp_socket_listen(tcp_socket socket) {
	if (listen(socket, 5))
		return log_error(30, tcp_errno);
	return 0;
}

int tcp_socket_accept(tcp_socket socket_listen, tcp_socket *socket_connection) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	int sockaddr_size = sizeof(sockaddr);
	*socket_connection = accept(socket_listen, &sockaddr, &sockaddr_size);
	#ifdef _WIN32
		if (*socket_connection==INVALID_SOCKET)
	#else
		if (*socket_connection<0)
	#endif
			return log_error(31, tcp_errno);
	return 0;
}

int tcp_set_socket_timeout(tcp_socket socket) {
	#ifdef _WIN32
		int timeout = TCP_TIMEOUT*1000;
	#else
		struct timeval timeout;
		timeout.tv_sec = TCP_TIMEOUT;
		timeout.tv_usec = 0;
	#endif
	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) || setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) )
		return log_error(32, tcp_errno);
	return 0;
}

int tcp_recv_str(tcp_socket socket, char *str, int str_size) {
	size_t str_len=0;
	do {
		int recv_len = recv(socket, str+str_len, str_size-str_len, 0);
		if (recv_len==0
		#ifdef _WIN32
			|| (recv_len<0 && tcp_errno==10060)
		#endif
		) {
			log_error(33, TCP_TIMEOUT);
			return -1;
		}
		if (recv_len<0) {
			log_error(45, tcp_errno);
			return -1;
		}
		if (str_len+recv_len>str_size)
			return log_error(5, str_size, str_len+recv_len);
		str_len += recv_len;
	} while(str[str_len-1]!=0);
	return 0;
}

int tcp_send(tcp_socket socket, char *data, int size) {
	while (size>0) {
		int send_len = send(socket, data, size, 0);
		if (send_len<=0)
			return log_error(34, tcp_errno);
		size -= send_len;
		data += send_len;
	}
	return 0;
}

int tcp_socket_close(tcp_socket socket) {
	#ifdef _WIN32
		if (closesocket(socket))
	#else
		if (close(socket))
	#endif
			return log_error(35, tcp_errno);
	return 0;
}
