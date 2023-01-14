#include <stdarg.h>
#include <stdlib.h>

#include "utils.h"

#define STREAM_SIZE_INIT 100*1024
#define STREAM_SIZE_STEP 10*1024*1024

int stream_init(stream *stream) {
	stream->data = malloc(STREAM_SIZE_INIT);
	if (stream->data==NULL)
		return log_error(9, STREAM_SIZE_INIT);
	stream->size = STREAM_SIZE_INIT-1;
	stream->data[0] = 0;
	stream->len = 0;
	return 0;
}

int stream_free(stream *stream) {
	if (stream->data==NULL)
		return log_error(51);
	free(stream->data);
	stream->data = NULL;
	stream->size = -1;
	stream->len = -1;
	return 0;
}

int stream_clear(stream *stream) {
	stream->len = 0;
	stream->data[0] = 0;
	return 0;
}


int stream_replace(stream *stream_dest, stream *stream_source) {
	if(stream_free(stream_dest)) return 1;
	stream_dest->data = stream_source->data;
	stream_dest->len  = stream_source->len;
	stream_dest->size = stream_source->size;
	return 0;
}

int _stream_resize(stream *stream, int size_need) {
	if (stream->data==NULL)
		return log_error(51);
	if (stream->len+size_need > stream->size) {
		int size_alloc = stream->len + size_need + STREAM_SIZE_STEP;
		char *data_new = malloc(size_alloc);
		if (data_new==NULL)
			return log_error(9, size_alloc);
		for (int i=0; i <= stream->len; i++)
			data_new[i] = stream->data[i];
		free(stream->data);
		stream->data = data_new;
		stream->size = size_alloc-1;
	}
	return 0;
}

int stream_add_substr(stream *stream, char *source, int pos_begin, int pos_end) {
	if (_stream_resize(stream, (pos_end - pos_begin) + 1)) return 1;
	for (int i = pos_begin; i <= pos_end; i++)
		stream->data[stream->len++] = source[i];
	stream->data[stream->len] = 0;
	return 0;
}


int stream_add_str(stream *stream, ...) {
	va_list args;
    va_start(args, &stream);
    while(1)  {
    	char *s = va_arg(args, char *);
    	if (s==NULL) break;
    	if (stream_add_substr(stream, s, 0, strlen(s)-1)) {
    		va_end(args);
    		return 1;
    	}
    }
    va_end(args);
    return 0;
}

int stream_add_char(stream *stream, char c) {
	if (stream->len>=stream->size && _stream_resize(stream, 1)) return 1;
	stream->data[stream->len++] = c;
	stream->data[stream->len] = 0;
    return 0;
}

int stream_add_substr_escaped(stream *stream, char *source, int pos_begin, int pos_end) {
	if (stream_add_char(stream, '"')) return 1;
	for(int pos=pos_begin; pos<=pos_end; pos++) {
		char c = source[pos];
		if (str_escaped_char(&c)==-1)
			if (stream_add_char(stream, '\\')) return 1;
		if (stream_add_char(stream, c)) return 1;
	}
	if (stream_add_char(stream, '"')) return 1;
    return 0;
}

int stream_add_str_escaped(stream *stream, char *str) {
    return stream_add_substr_escaped(stream, str, 0, strlen(str)-1);
}

int stream_add_substr_unescaped(stream *stream, char *source, int pos_begin, int pos_end) {
	if (source[pos_begin]!='"') {
		return stream_add_substr(stream, source, pos_begin, pos_end);
	}
	int pos=pos_begin+1;
	int esc = 0;
	for(; pos<pos_end; pos++) {
		if (!esc && source[pos]=='\\') {
			esc=1;
			continue;
		}
		char c = source[pos];
		if (esc) {
			str_unescaped_char(&c);
			esc=0;
		}
		if (stream_add_char(stream, c)) return 1;
	}
	return 0;
}

int stream_add_int(stream *stream, int value) {
	char str[50];
	sprintf(str, "%d", value);
	return stream_add_str(stream, str, NULL);
}

int stream_add_rpad(stream *stream, char *prefix, char *str, int str_len_max, char *suffix) {
	if (stream_add_str(stream, prefix, str, suffix, NULL)) return 1;
	int len = str_len_max-strlen(str);
	for(int i=0; i<len; i++)
		if (stream_add_char(stream, ' ')) return 1;
    return 0;
}

