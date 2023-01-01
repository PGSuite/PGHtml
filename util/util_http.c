#include "utils.h"

char *http_status_desc(int status_code) {
	if (status_code==HTTP_STATUS_OK)             return "OK";
	if (status_code==HTTP_STATUS_BAD_REQUEST)    return "Bad Request";
	if (status_code==HTTP_STATUS_NOT_FOUND)      return "Not Found";
	if (status_code==HTTP_STATUS_INTERNAL_ERROR) return "Internal Server Error";
	return "<Status description unknown>";
}

char *http_status_error_text(int status_code) {
	if (status_code==HTTP_STATUS_OK)             return "<No error>";
	if (status_code==HTTP_STATUS_BAD_REQUEST)    return "Incorrect request format, see server log";
	if (status_code==HTTP_STATUS_NOT_FOUND)      return "Page not found";
	if (status_code==HTTP_STATUS_INTERNAL_ERROR) return "Request processing error on server, see server log";
	return "<Error unknown>";
}

int _http_recv_var(tcp_socket socket_connection, char *buffer, int buffer_len, int *i, char char_end, char *var, int *var_len, int var_size, int *var_received) {
	for(;*i<buffer_len;(*i)++) {
		if (buffer[*i]=='\r') continue;
		if (buffer[*i]==char_end) {
			var[*var_len]=0;
			*var_len = 0;
			*var_received = 1;
			(*i)++;
			return 0;
		}
		if (*var_len+2>var_size) {
			log_stderr_print(5, var_size, *var_len+2);
			http_send_error(socket_connection, HTTP_STATUS_INTERNAL_ERROR);
			return 1;
		}
		var[(*var_len)++]=buffer[*i];
	}
	return 0;
}

int http_recv_request(tcp_socket socket_connection, http_request *request) {
	char buffer[1024*1024];
	char header[128];
	int header_len = 0;
	int content_len = 0;
	int received_method = 0;
	int received_path = 0;
	int received_protocol = 0;
	int received_headers = 0;
	int var_len = 0;
	request->content.data = NULL;
	do {
		int buffer_len = recv(socket_connection, buffer, sizeof(buffer), 0);
		if (buffer_len==0
		#ifdef _WIN32
			|| (buffer_len<0 && tcp_errno==10060)
		#endif
		) {
			log_stderr_print(33, TCP_TIMEOUT);
			return -1;
		}
		if (buffer_len<0) {
			log_stderr_print(45, tcp_errno);
			return -1;
		}
		int i=0;
		if (!received_method   && _http_recv_var(socket_connection, buffer, buffer_len, &i, ' ',  &request->method,   &var_len, sizeof(request->method),   &received_method))   return 1;
		if (!received_path     && _http_recv_var(socket_connection, buffer, buffer_len, &i, ' ',  &request->path,     &var_len, sizeof(request->path),     &received_path))     return 1;
		if (!received_protocol && _http_recv_var(socket_connection, buffer, buffer_len, &i, '\n', &request->protocol, &var_len, sizeof(request->protocol), &received_protocol)) return 1;
		if (!received_headers) {
			for(;i<buffer_len;i++) {
				if (buffer[i]=='\r') continue;
				if (buffer[i]=='\n') {
					if (header_len==0) {
						if (stream_init(&request->content)) {
							http_send_error(socket_connection, HTTP_STATUS_INTERNAL_ERROR);
							return 1;
						}
						received_headers = 1;
						i++;
						break;
					}
					int pos = 0;
					for(;pos<header_len;pos++)
						if (header[pos]==':') break;
					if (!strncmp(header, "Content-Length", pos)) {
						content_len = atoi(header+pos+1);
					}
					header[header_len=0]=0;
					continue;
				}
				if (header_len==sizeof(header)-1) {
					for(int j=0; j<64; j++)
						header[j] = header[header_len-64+j];
					header_len = 64;
				}
				header[header_len++] = buffer[i];
				header[header_len] = 0;
			}
			if (received_headers && i<buffer_len) {
				if (stream_add_substr(&request->content, buffer, i, buffer_len-1)) {
					stream_free(request->content);
					http_send_error(socket_connection, HTTP_STATUS_INTERNAL_ERROR);
					return 1;
				}
			}
		}
	} while(!received_headers || request->content.len<content_len);
	log_stdout_println("request received, method: %s, path: \"%s\", content length: %d", request->method, request->path, request->content.len);
	return 0;
}

int http_send_error(tcp_socket socket_connection, int error_code) {

	char html[32*1024];
	if (str_format(html, sizeof(html), "<html><head><title>Error %d</title></head><body>%d %s<br>%s</body></html>", error_code, error_code, http_status_desc(error_code), http_status_error_text(error_code)))
		return 1;

	return http_send_response(socket_connection, error_code, HTTP_CONTENT_TYPE_HTML, strlen(html), html);

}

int http_send_response(tcp_socket socket_connection, int status_code, char *content_type, int content_length, char *content) {

	char header[STR_SIZE];
	if (str_format(header, sizeof(header), "HTTP/1.1 %d %s\r\nContent-Type: %s; charset=UTF-8\r\nContent-Length: %d\r\n\r\n", status_code, http_status_desc(status_code), content_type, content_length)) {
		http_send_error(socket_connection, 500);
		return 1;
	}

	if (tcp_send(socket_connection, header, strlen(header))) return 1;

	if (tcp_send(socket_connection, content, content_length)) return 1;

	log_stdout_println("response sent, status: %d", status_code);

	return 0;

}
