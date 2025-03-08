#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32

#include <ws2tcpip.h>

#else

#include <sys/un.h>
#include <sys/file.h>

#endif

#include "util.h"

#define TCP_WIN_UNIX_HOST_ADDR "127.0.0.1"

char tcp_host_name[256] = "";
char tcp_host_addr[32]  = "";

int tcp_startup() {
    #ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2,2), &wsa_data))
			return log_error(27, WSAGetLastError());
		// log_info("Windows Socket Architecture (WSA) started, status: \"%s\"", wsa_data.szSystemStatus);
	#endif
	return 0;
}

int tcp_get_host_info() {
	if (gethostname(tcp_host_name, sizeof(tcp_host_name))) return log_warn(903, tcp_errno);
	struct hostent *host_ent;
	host_ent = gethostbyname(tcp_host_name);
	if (host_ent==NULL)  perror("");
	if (host_ent==NULL) return log_warn(903, tcp_errno);
	if (host_ent->h_addrtype==AF_INET && host_ent->h_addr!=NULL)
	if (str_copy(tcp_host_addr, sizeof(tcp_host_addr), inet_ntoa (*(struct in_addr*)host_ent->h_addr))) return 1;
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


int tcp_connect(tcp_socket sock, char *addr, int port) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_addr.s_addr = inet_addr(addr);
	sockaddr.sin_port = htons(port);
	sockaddr.sin_family = AF_INET;
    if(connect(sock, &sockaddr, sizeof(sockaddr)))
    	return log_error(43, addr, port, tcp_errno);
    return 0;
}

int tcp_bind(tcp_socket sock, char *addr, int port) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	if (addr!=NULL)
		sockaddr.sin_addr.s_addr = inet_addr(addr);
	else
		sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_family = AF_INET;
	if (bind(sock, &sockaddr, sizeof(sockaddr)))
		return log_error(29, port, tcp_errno);
	return 0;
}

int tcp_socket_listen(tcp_socket sock) {
	if (listen(sock, 10))
		return log_error(30, tcp_errno);
	return 0;
}

int tcp_socket_accept(tcp_socket socket_listen, tcp_socket *socket_connection) {
	/*
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	int sockaddr_size = sizeof(sockaddr);
	*socket_connection = accept(socket_listen, &sockaddr, &sockaddr_size);
	*/
	*socket_connection = accept(socket_listen, NULL, NULL);
	#ifdef _WIN32
		if (*socket_connection==INVALID_SOCKET)
	#else
		if (*socket_connection<0)
	#endif
			return log_error(31, tcp_errno);
	return 0;
}

int tcp_set_socket_timeout(tcp_socket sock) {
	#ifdef _WIN32
		int timeout = TCP_TIMEOUT*1000;
	#else
		struct timeval timeout;
		timeout.tv_sec = TCP_TIMEOUT;
		timeout.tv_usec = 0;
	#endif
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) || setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) )
		return log_error(32, tcp_errno);
	return 0;
}

int tcp_recv_str(tcp_socket sock, char *str, int str_size) {
	size_t str_len=0;
	while(1) {
		int recv_len = recv(sock, str+str_len, str_size-str_len, 0);
		if (recv_len==0
		#ifdef _WIN32
			|| (recv_len<0 && tcp_errno==10060)
		#endif
		) {
			return log_warn(901, TCP_TIMEOUT);
		}
		if (recv_len<0) {
			return log_warn(902, tcp_errno);
		}
		if (str_len+recv_len>str_size)
			return log_error(5, str_size, str_len+recv_len);
		for(; recv_len; recv_len--,str_len++)
			if (str[str_len]==0) return 0;
	}
	return 0;
}

int tcp_send(tcp_socket sock, char *data, int size) {
	while (size>0) {
		int send_len = send(sock, data, size, 0);
		if (send_len<=0)
			return log_error(34, tcp_errno);
		size -= send_len;
		data += send_len;
	}
	return 0;
}

int tcp_socket_close(tcp_socket sock) {
	if (sock==0)
		return 0;
	#ifdef _WIN32
		if (closesocket(sock))
	#else
		if (close(sock))
	#endif
			return log_error(35, tcp_errno);
	sock = 0;
	return 0;
}

int tcp_addr_hostname(char *addr, int addr_size, const char *hostname) {
	if (tcp_startup()) return 1;
	struct addrinfo *addr_info = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	int eai_errno;
	if (eai_errno=getaddrinfo(hostname, NULL, &hints, &addr_info))
		return log_error(93, hostname, "eai_errno", eai_errno);
	struct sockaddr_in *saddr_in = addr_info->ai_addr;
	int res = inet_ntop(addr_info->ai_family, &(saddr_in->sin_addr), addr, addr_size)==NULL;
	if (res) log_error(93, hostname, "errno", errno);
	freeaddrinfo(addr_info);
	return res;
}

int tcp_unix_socket_create(tcp_socket *sock) {
	#ifdef _WIN32
		return tcp_socket_create(sock);
	#else
		*sock = socket(AF_UNIX, SOCK_STREAM, IPPROTO_IP);
		if(*sock < 0) return log_error(28, tcp_errno);
		return 0;
	#endif
}

#ifndef _WIN32
int _tcp_unix_sockaddr_path(char *path, int path_size, int port) {
	char program_name[STR_SIZE];
	if (log_get_program_name(program_name, sizeof(program_name))) return 1;
	return str_format(path, path_size, "/tmp/.s.%s.%d", program_name, port);
}

int _tcp_unix_sockaddr_create(struct sockaddr_un *saddr, int port) {
	memset(saddr, 0, sizeof(struct sockaddr_un));
	saddr->sun_family = AF_UNIX;
	if (_tcp_unix_sockaddr_path(saddr->sun_path, sizeof(saddr->sun_path), port))
		return 1;
	return 0;
}
#endif

int tcp_unix_bind(tcp_socket sock, int port) {
	#ifdef _WIN32
		if (tcp_bind(sock, TCP_WIN_UNIX_HOST_ADDR, port))
			return 1;
		log_info("binded to %s:%d", TCP_WIN_UNIX_HOST_ADDR, port);
		return 0;
	#else
		struct sockaddr_un saddr;
		char lock_path[STR_SIZE];
		if (_tcp_unix_sockaddr_create(&saddr, port))
			return 1;
		if (_tcp_unix_sockaddr_path(lock_path, sizeof(lock_path), port))
			return 1;
		if (str_add(lock_path, sizeof(lock_path), ".lock", NULL))
			return 1;
		int lock_fd = open(lock_path, O_CREAT, S_IRWXU);
		if(lock_fd<0)
			return log_error(8, lock_path, errno);
		if (flock(lock_fd, LOCK_EX|LOCK_NB))
			return log_error(87, lock_path, errno);
		if (file_remove(saddr.sun_path, 0))
			return 1;
		if (bind(sock, &saddr, sizeof(saddr)))
			return log_error(84, saddr.sun_path, tcp_errno);
		log_info("binded to unix socket \"%s\"", saddr.sun_path);
		return 0;
	#endif
}

int tcp_unix_connect(tcp_socket sock, int port) {
	#ifdef _WIN32
		return tcp_connect(sock, TCP_WIN_UNIX_HOST_ADDR, port);
	#else
		struct sockaddr_un saddr;
		if (_tcp_unix_sockaddr_create(&saddr, port))
			return 1;
	    if(connect(sock, &saddr, sizeof(saddr)))
	    	return log_error(85, saddr.sun_path, tcp_errno);
	    return 0;
	#endif
}

