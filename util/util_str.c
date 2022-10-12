#include <stdarg.h>
#include <strings.h>

#include "util_str.h"


int str_find(char *body, int body_size, int pos, char *substr, int ignore_case) {
	int substr_length = strlen(substr);
	for(int i=pos; i<=(body_size-substr_length); i++) {
		int find = 1;
		for(int j=0; j<substr_length; j++) {
			if (!ignore_case && body[i+j]!=substr[j]) { find=0; break; }
			if (ignore_case && tolower(body[i+j])!=tolower(substr[j])) { find=0; break; }

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
		log_stderr_print(54, "str_format");
		return 1;
	}

	int len_dest=strlen(dest);

	if (len!=strlen(dest)) {
		log_stderr_print( 5, dest_size, len+1);
		return 1;
	}

    va_end(args);
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

void str_delete_char(char *str, char c) {
	size_t len = strlen(str);
	for(int i=0, pos=0; i<=len; i++)
		if (str[i]!=c) str[pos++] = str[i];
}

int str_tag_attributes(char *tag, STR_MAP *attributes) {
	int pos_begin,pos_end;
	int tag_len = strlen(tag);
	int tag_attributes_size_max = sizeof(attributes->keys)/sizeof(attributes->keys[0]);
	attributes->size = 0;
	for(int pos=0; pos<tag_len; pos++) {
		if (tag[pos]=='=') {
			if (attributes->size==tag_attributes_size_max) {
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
			if (str_substr(attributes->keys[attributes->size], sizeof(attributes->keys[0]), tag, pos_begin, pos_end))
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
			if (str_substr(attributes->values[attributes->size], sizeof(attributes->values[0]), tag, pos_begin+1, pos_end-1))
				return 1;
			attributes->size++;
		}
	}
	return 0;
}

void str_map_clear(STR_MAP *map) {
	map->size = 0;
}

int str_map_index(STR_MAP *map, char *key) {
	for (int i=0; i<map->size; i++)
		if (strcmp(map->keys[i], key)==0) return i;
	return -1;
}

int str_map_put(STR_MAP *map, char *key, char *value) {
	int index = str_map_index(map, key);
	if (index==-1) {
		if (map->size>=STR_COLLECTION_SIZE_MAX) {
			log_stderr_print(24, STR_COLLECTION_SIZE_MAX, key, value);
			return 1;
		}
		index = map->size++;
	}
	if (str_copy(map->keys[index], sizeof(map->keys[0]), key))
		return 1;
	if (str_copy(map->values[index], sizeof(map->values[0]), value))
		return 1;
	return 0;
}

void str_list_clear(STR_LIST *list) {
	list->size = 0;
}

int str_list_add(STR_LIST *list, char *value) {
	if (list->size>=STR_COLLECTION_SIZE_MAX) {
		log_stderr_print(20, STR_COLLECTION_SIZE_MAX, value);
		return 1;
	}
	if (str_copy(list->values[list->size], sizeof(list->values[0]), value))
		return 1;
	list->size++;
	return 0;
}


int str_list_split(STR_LIST *list, char *values, char delimeter) {
	str_list_clear(list);
	int values_len = strlen(values);
	char value[STR_COLLECTION_VALUE_SIZE_MAX];
	int value_size = 0;
	value[value_size]=0;
	for (int i=0; values[i]!=0; i++) {
		if (values[i]==delimeter) {
			if (str_list_add(list, value)) return 1;
			value_size=0;
		} else {
			if (value_size>=STR_COLLECTION_VALUE_SIZE_MAX-1) {
				log_stderr_print(21, STR_COLLECTION_VALUE_SIZE_MAX, value);
				return 1;
			}
			value[value_size++]=values[i];
		}
		value[value_size]=0;
	}
	if (str_list_add(list, value)) return 1;
	return 0;
}

int str_list_agg(STR_LIST *list, char *values, int values_size, char delimeter) {
	int values_len = 0;
    for(int i=0; i<list->size; i++) {
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

int str_list_print(STR_LIST *list, char delimeter, char *prefix) {
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


/*
	FILE_BODY f;
	file_read("r:\\t.txt", &f);
	printf("%d %s\n", f.size, f.body );

	for(int i=0; i<f.size;) {
		printf("%d\n", i);
		if(str_utf8_next(f.body, &i)) return 1;
	}
*/
