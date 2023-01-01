#include <stdarg.h>
#include <strings.h>

#include "utils.h"

int str_find(char *source, int source_size, int pos, char *substr, int ignore_case) {
	int substr_length = strlen(substr);
	for(int i=pos; i<=(source_size-substr_length); i++) {
		int find = 1;
		for(int j=0; j<substr_length; j++) {
			if (!ignore_case && source[i+j]!=substr[j]) { find=0; break; }
			if (ignore_case && tolower(source[i+j])!=tolower(substr[j])) { find=0; break; }

		}
		if (find) return i;
	}
	return -1;
}

int str_find_char_html(char *html, int html_size, int pos, char c) {
	char in_str_char = 0;
	for(int i=pos; i<html_size; i++) {
		if (!in_str_char && html[i]==c) return i;
		if (!in_str_char && (html[i]=='"' || html[i]=='\'')) {
			in_str_char = html[i];
			continue;
		}
		if (in_str_char && in_str_char==html[i]) {
			in_str_char = 0;
			continue;
		}
	}
	return -1;
}

int str_substr(char *dest, int dest_size, char *source, int pos_begin, int pos_end) {
	if(pos_begin<0 || pos_end<0 || pos_begin>pos_end) {
		log_stderr_print(54, "str_substr");
		return 1;
	}
	int len = pos_end-pos_begin+1;
	if (len>=dest_size) {
		log_stderr_print(5, dest_size, len+1);
		return 1;
	}
	for(int i=0; i<len; i++)
		dest[i] = source[pos_begin+i];
	dest[len] = 0;
	return 0;
}

int str_copy(char *dest, int dest_size, char *source) {
	int source_len = strlen(source);
	if ( source_len>=dest_size ) {
		log_stderr_print(5, dest_size, source_len+1);
		return 1;
	}
	for(int i=0; i<=source_len; i++) {
		dest[i] = source[i];
	}
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
    		log_stderr_print(5, dest_size, dest_len+len+1);
    		return 1;
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

	if (len<0) {
		va_end(args);
		log_stderr_print(54, "str_format");
		return 1;
	}

	int len_dest=strlen(dest);

	if (len!=strlen(dest)) {
		va_end(args);
		log_stderr_print( 5, dest_size, len+1);
		return 1;
	}

    va_end(args);
    return 0;
}


int str_insert_char(char *str, int str_size, int pos, char c) {
	int len = strlen(str);
	if (len+1>=str_size) {
		log_stderr_print(5, str_size, len+2);
		return 1;
	}
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
	if (len>=dest_size) {
		log_stderr_print(5, dest_size, len+1);
		return 1;
	}
	int i = strlen(dest);
	if (i>=len) return 0;
	while(i<len)
		dest[i++]=' ';
	dest[len]=0;
    return 0;
}

int str_tag_attributes(char *tag, str_map *attributes) {
	int pos_begin,pos_end;
	int tag_len = strlen(tag);
	int tag_attributes_size_max = sizeof(attributes->keys)/sizeof(attributes->keys[0]);
	attributes->len = 0;
	for(int pos=0; pos<tag_len; pos++) {
		if (tag[pos]=='=') {
			if (attributes->len==tag_attributes_size_max) {
				log_stderr_print(6, tag);
				return 1;
			}
			for(pos_end = pos-1; pos_end>0 && (tag[pos_end]==' ' || tag[pos_end]=='\t');  pos_end--);
			for(pos_begin = pos_end-1; pos_begin>0 && tag[pos_begin]!=' ' && tag[pos_begin]!='\t';  pos_begin--);
			if (pos_begin==0 || pos_end==0) {
				log_stderr_print(7, pos, tag);
				return 1;
			}
			pos_begin++;
			if (str_substr(attributes->keys[attributes->len], sizeof(attributes->keys[0]), tag, pos_begin, pos_end))
				return 1;
			for(pos_begin = pos+1; pos_begin<tag_len && (tag[pos_begin]==' ' || tag[pos_begin]=='\t');  pos_begin++);
			if (pos_begin==tag_len || (tag[pos_begin]!='"' && tag[pos_begin]!='\'')  ) {
				log_stderr_print(7, pos, tag);
				return 1;
			}
			for(pos_end = pos_begin+1; pos_end<tag_len && tag[pos_end]!=tag[pos_begin]; pos_end++);
			if (pos_end==tag_len) {
				log_stderr_print(7, pos, tag);
				return 1;
			}
			if (str_substr(attributes->values[attributes->len], sizeof(attributes->values[0]), tag, pos_begin+1, pos_end-1))
				return 1;
			attributes->len++;
		}
	}
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
		if (map->len>=STR_COLLECTION_SIZE) {
			log_stderr_print(24, STR_COLLECTION_SIZE, key, value);
			return 1;
		}
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
	if (list->len>=STR_COLLECTION_SIZE) {
		log_stderr_print(20, STR_COLLECTION_SIZE, value);
		return 1;
	}
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
			if (value_size>=STR_COLLECTION_VALUE_SIZE-1) {
				log_stderr_print(21, STR_COLLECTION_VALUE_SIZE, data);
				return 1;
			}
			value[value_size++]=data[pos];
		}
		value[value_size]=0;
	}
	if (str_list_add(list, value)) return 1;
	return 0;
}

/*
int str_list_split(str_list *list, char *values, char delimeter) {
	str_list_clear(list);
	int values_len = strlen(values);
	char value[STR_COLLECTION_VALUE_SIZE];
	int value_size = 0;
	value[value_size]=0;
	for (int i=0; values[i]!=0; i++) {
		if (values[i]==delimeter) {
			if (str_list_add(list, value)) return 1;
			value_size=0;
		} else {
			if (value_size>=STR_COLLECTION_VALUE_SIZE-1) {
				log_stderr_print(21, STR_COLLECTION_VALUE_SIZE, value);
				return 1;
			}
			value[value_size++]=values[i];
		}
		value[value_size]=0;
	}
	if (str_list_add(list, value)) return 1;
	return 0;
}
*/


int str_list_agg(str_list *list, char *values, int values_size, char delimeter) {
	int values_len = 0;
    for(int i=0; i<list->len; i++) {
    	if (i>0) {
    		if (values_len>=values_size-1) {
    			log_stderr_print(5, values_size, values_size+1);
    			return 1;
    		}
    		values[values_len++] = delimeter;
    	}
    	for(int j=0; list->values[i][j]; j++) {
    		if (values_len>=values_size-1) {
    			log_stderr_print(5, values_size, values_size+1);
    			return 1;
    		}
    		values[values_len++] = list->values[i][j];
    	}
    }
    values[values_len] = 0;
	return 0;
}

int str_list_print(str_list *list, char delimeter, char *prefix) {
	char values[32000];
	if (str_list_agg(list, values, sizeof(values), delimeter)) return 1;
	log_stdout_printf("%s%s", prefix, values);
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
	else {
		log_stderr_print(15, *i, str);
		return 1;
	}
	for(int j=1; j<len; j++)
		if ((str[*i+j] & 0xC0) != 0x80) {
			log_stderr_print(53, *i, j, str);
			return 1;
		}
	*i = *i+len;
	return 0;
}

void str_len_max(int *len_max, char *str) {
	int len = strlen(str);
	if (len>*len_max) *len_max = len;
}


