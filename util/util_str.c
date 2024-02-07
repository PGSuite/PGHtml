#include <stdarg.h>
#include <strings.h>

#include "utils.h"

int str_find(char *source, int pos, char *substr, int ignore_case) {
	int substr_length = strlen(substr);
	for(int i=pos; 1; i++) {
		int find = 1;
		for(int j=0; j<substr_length; j++) {
			if (!source[i+j]) return -1;
			if (!ignore_case && source[i+j]!=substr[j]) { find=0; break; }
			if (ignore_case && tolower(source[i+j])!=tolower(substr[j])) { find=0; break; }
		}
		if (find) return i;
	}
	return -1;
}

int str_substr(char *dest, int dest_size, char *source, int pos_begin, int pos_end) {
	if(pos_begin<0 || pos_end<0 || pos_begin>pos_end)
		return log_error(54, "str_substr");
	int len = pos_end-pos_begin+1;
	if (len>=dest_size)
		return log_error(5, dest_size, len+1);
	for(int i=0; i<len; i++)
		dest[i] = source[pos_begin+i];
	dest[len] = 0;
	return 0;
}

int str_copy(char *dest, int dest_size, char *source) {
	if(dest==NULL) return log_error(45, "dest", "str_copy");
	if(source==NULL) return log_error(45, "source", "str_copy");
	int source_len = strlen(source);
	if (source_len>=dest_size)
		return log_error(5, dest_size, source_len+1);
	for(int i=0; i<=source_len; i++)
		dest[i] = source[i];
	return 0;
}

int str_copy_more(char *dest, int dest_size, char *source) {
	if(dest==NULL) return log_error(45, "dest", "str_copy_more");
	if(source==NULL) return log_error(45, "source", "str_copy_more");
	if (dest_size<10)
		return log_error(5, dest_size, 10);
	int source_len = strlen(source);
	if (source_len>=dest_size) {
		for(int i=0; i<dest_size-4; i++)
			dest[i] = source[i];
		dest[dest_size-4] = '.';
		dest[dest_size-3] = '.';
		dest[dest_size-2] = '.';
		dest[dest_size-1] = 0;
	} else
		for(int i=0; i<=source_len; i++)
			dest[i] = source[i];
	return 0;
}

int str_add(char *dest, int dest_size, ...) {

	int dest_len = strlen(dest);

	va_list args;
    va_start(args, &dest_size);

    while(1)  {
    	char *s = va_arg(args, char *);
    	if (s==NULL) break;
    	int len = strlen(s);
    	if ( (dest_len+len)>=dest_size ) {
    	    va_end(args);
    	    return log_error(5, dest_size, dest_len+len+1);
    	}
    	for(int i=0; i<len; i++) {
    		dest[dest_len++] = s[i];
    	}
    }

    dest[dest_len] = 0;

    va_end(args);
    return 0;
}

int str_format(char *dest, int dest_size, char *format, ...) {
	va_list args;
    va_start(args, &format);
	int len = vsnprintf(dest, dest_size, format, args);
	va_end(args);
	if (len<0) return log_error(54, format);
	if (len!=strlen(dest)) return log_error(5, dest_size, len+1);
    return 0;
}

int str_add_format(char *dest, int dest_size, char *format, ...) {
	char str[10*1024];
	va_list args;
    va_start(args, &format);
	int len = vsnprintf(str, sizeof(str), format, args);
	va_end(args);
	if (len<0) return log_error(54, format);
	if (len!=strlen(str)) return log_error( 5, sizeof(str), len+1);
    return str_add(dest, dest_size, str, NULL);
}



int str_insert_char(char *str, int str_size, int pos, char c) {
	int len = strlen(str);
	if (len+1>=str_size)
		return log_error(5, str_size, len+2);
	for(int i=len; i>=pos; i--)
		str[i+1] = str[i];
	str[pos] = c;
	return 0;
}

int str_delete_char(char *str, int pos) {
	for(int i=pos; str[i]; i++)
		str[i] = str[i+1];
	return 0;
}

void str_replace_char(char *str, char c_old, char c_new) {
	for(int i=0; str[i]; i++)
		if (str[i]==c_old) str[i]=c_new;
}


int str_escaped_char(char *c) {
	if (*c=='"' || *c=='\\')  return -1;
	if (*c=='\r') { *c='r'; return -1; }
	if (*c=='\n') { *c='n'; return -1; }
	if (*c=='\t') { *c='t'; return -1; }
	return 0;
}

int str_unescaped_char(char *c) {
	if (*c=='r') { *c='\r'; return 0; }
	if (*c=='n') { *c='\n'; return 0; }
	if (*c=='t') { *c='\t'; return 0; }
	if (*c!='"' && *c!='\\') *c='?';
	return 0;
}

int str_escaped(char *str, int str_size) {
	if(str_insert_char(str, str_size, 0, '"')) return 1;
	int i=1;
	for(; str[i]; i++) {
		char c = str[i];
		if (str_escaped_char(&c)==-1) {
			if (str_insert_char(str, str_size, i++, '\\')) return 1;
			str[i] = c;
			continue;
		}
	}
	if(str_insert_char(str, str_size, i, '"')) return 1;
	return 0;
}

int str_unescaped(char *str) {
	if (str[0]!='"') return 0;
	if (str_delete_char(str, 0)) return 1;
	int i=0;
	int esc = 0;
	while(str[i]) {
		if (!esc && str[i]=='\\') {
			if (str_delete_char(str, i)) return 1;
			esc=1;
			continue;
		}
		if (esc) {
			str_unescaped_char(&str[i]);
			esc=0;
		}
		i++;
	}
	if (i>0 && str[i-1]=='"')
		str_delete_char(str, i-1);
	return 0;
}

int str_rtrim(char *dest, int dest_size, int len) {
	if (len>=dest_size)
		return log_error(5, dest_size, len+1);
	int i = strlen(dest);
	if (i>=len) return 0;
	while(i<len)
		dest[i++]=' ';
	dest[len]=0;
    return 0;
}

void str_map_clear(str_map *map) {
	map->len = 0;
}

int str_map_index(str_map *map, char *key) {
	for (int i=0; i<map->len; i++)
		if (strcmp(map->keys[i], key)==0) return i;
	return -1;
}

int str_map_put(str_map *map, char *key, char *value) {
	int index = str_map_index(map, key);
	if (index==-1) {
		if (map->len>=STR_COLLECTION_SIZE)
			return log_error(24, STR_COLLECTION_SIZE, key, value);
		index = map->len++;
	}
	if (str_copy(map->keys[index], sizeof(map->keys[0]), key))
		return 1;
	if (str_copy(map->values[index], sizeof(map->values[0]), value))
		return 1;
	return 0;
}

void str_list_clear(str_list *list) {
	list->len = 0;
}

int str_list_add(str_list *list, char *value) {
	if (list->len>=STR_COLLECTION_SIZE)
		return log_error(20, STR_COLLECTION_SIZE, value);
	if (str_copy(list->values[list->len], sizeof(list->values[0]), value))
		return 1;
	list->len++;
	return 0;
}

int str_list_split(str_list *list, char *data, int pos_begin, int pos_end, char delimeter) {
	if (pos_end==-1) pos_end = strlen(data+pos_begin)-1;
	str_list_clear(list);
	char value[STR_COLLECTION_VALUE_SIZE];
	int value_size = 0;
	value[value_size]=0;
	for (int pos=pos_begin; pos<=pos_end; pos++) {
		if (data[pos]==delimeter) {
			if (str_list_add(list, value)) return 1;
			value_size=0;
		} else {
			if (value_size>=STR_COLLECTION_VALUE_SIZE-1)
				return log_error(21, STR_COLLECTION_VALUE_SIZE, data);
			value[value_size++]=data[pos];
		}
		value[value_size]=0;
	}
	if (str_list_add(list, value)) return 1;
	return 0;
}

int str_utf8_next(char *str, int *i) {

	if ((str[*i] & 0x80) == 0x00) {
		(*i)++;
		return 0;
	};

	int len;
	if ((str[*i] & 0xE0) == 0xC0) len=2;
	else if ((str[*i] & 0xF0) == 0xE0) len=3;
	else if ((str[*i] & 0xF8) == 0xF0) len=4;
	else if ((str[*i] & 0xFC) == 0xF8) len=5;
	else if ((str[*i] & 0xFE) == 0xFC) len=6;
	else return log_error(15, *i, str);
	for(int j=1; j<len; j++)
		if ((str[*i+j] & 0xC0) != 0x80)
			return log_error(53, *i, j, str);
	*i = *i+len;
	return 0;
}

void str_len_max(int *len_max, char *str) {
	int len = strlen(str);
	if (len>*len_max) *len_max = len;
}


