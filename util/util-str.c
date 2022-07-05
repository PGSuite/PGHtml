#include <stdarg.h>
#include <strings.h>

#include "util-str.h"

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

int str_substr(char *dest, int dest_size, char *source, int pos, int len) {
	if (len<=0) {
		dest[0] = 0;
		return 0;
	}
	if (len>=dest_size) {
		stderr_printf(5, dest_size, len+1);
		return 1;
	}
	for(int i=0; i<len; i++) {
		dest[i] = source[pos+i];
	}
	dest[len] = 0;
	return 0;
}

int str_copy(char *dest, int dest_size, char *source) {
	int source_len = strlen(source);
	if ( source_len>=dest_size ) {
		stderr_printf(5, dest_size, source_len+1);
		return 1;
	}
	for(int i=0; i<=source_len; i++) {
		dest[i] = source[i];
	}
	return 0;
}

int str_add(char *dest, int dest_size, char *source) {
	int dest_len = strlen(dest);
	int source_len = strlen(source);
	if ( (dest_len+source_len)>=dest_size ) {
		stderr_printf(5, dest_size, dest_len+source_len+1);
		return 1;
	}
	for(int i=0; i<=source_len; i++) {
		dest[dest_len+i] = source[i];
	}
	return 0;
}

int str_add_n(char *dest, int dest_size, int n, ...) {
	char *s;
    va_list args;
    va_start(args, &n);

    for(int i=0; i<n; i++) {
    	s = va_arg(args, char *);

    	if (str_add(dest, dest_size, s)!=0) return 1;
    }
    va_end(args);
    return 0;
}

int str_rtrim(char *dest, int dest_size, int len) {
	if (len>=dest_size) {
		stderr_printf(5, dest_size, len+1);
		return 1;
	}
	int i = strlen(dest);
	if (i>=len) return 0;
	while(i<len)
		dest[i++]=' ';
	dest[len]=0;
    return 0;
}

int str_tag_attributes(char *tag, VARS *vars) {
	int pos_begin,pos_end;
	int tag_len = strlen(tag);
	int tag_attributes_size_max = sizeof(vars->names)/sizeof(vars->names[0]);
	vars->size = 0;
	for(int pos=0; pos<tag_len; pos++) {
		if (tag[pos]=='=') {
			if (vars->size==tag_attributes_size_max) {
				stderr_printf(6, tag);
				return 1;
			}
			for(pos_end = pos-1; pos_end>0 && (tag[pos_end]==' ' || tag[pos_end]=='\t');  pos_end--);
			for(pos_begin = pos_end-1; pos_begin>0 && tag[pos_begin]!=' ' && tag[pos_begin]!='\t';  pos_begin--);
			if (pos_begin==0 || pos_end==0) {
				stderr_printf(7, pos, tag);
				return 1;
			}
			pos_begin++;
			if (str_substr(vars->names[vars->size], sizeof(vars->names[0]), tag, pos_begin, pos_end-pos_begin+1))
				return 1;
			for(pos_begin = pos+1; pos_begin<tag_len && (tag[pos_begin]==' ' || tag[pos_begin]=='\t');  pos_begin++);
			if (pos_begin==tag_len || (tag[pos_begin]!='"' && tag[pos_begin]!='\'')  ) {
				stderr_printf(7, pos, tag);
				return 1;
			}
			for(pos_end = pos_begin+1; pos_end<tag_len && tag[pos_end]!=tag[pos_begin]; pos_end++);
			if (pos_end==tag_len) {
				stderr_printf(7, pos, tag);
				return 1;
			}
			if (str_substr(vars->values[vars->size], sizeof(vars->values[0]), tag, pos_begin+1, pos_end-pos_begin-1))
				return 1;
			vars->size++;
		}
	}
	return 0;
}

int str_vars_add(VARS *vars, char *name, char *value) {
	if (str_substr(vars->names[vars->size], sizeof(vars->names[0]), name, 0, strlen(name)))
		return 1;
	if (str_substr(vars->values[vars->size], sizeof(vars->values[0]), value, 0, strlen(value)))
		return 1;
	vars->size++;
	return 0;
}

int str_split(char array[ARRAY_SIZE_MAX][ARRAY_ELEMENT_SIZE_MAX], int *array_size, char *list, char delimeter) {
	int list_len = strlen(list);
	int element_index = 0;
	int element_size = 0;
	array[element_index][element_size]=0;
	for (int i=0; i<list_len; i++) {
		if (list[i]==delimeter) {
			if (element_index>=ARRAY_SIZE_MAX) {
				stderr_printf(20, ARRAY_SIZE_MAX, list);
				return 1;
			}
			element_index++;
			element_size=0;
			array[element_index][element_size]=0;
		} else {
			if (element_size>=ARRAY_ELEMENT_SIZE_MAX-1) {
				stderr_printf(21, ARRAY_ELEMENT_SIZE_MAX, array[element_index]);
				return 1;
			}
			array[element_index][element_size++]=list[i];
			array[element_index][element_size]=0;
		}
	}
	*array_size = element_index+1;
	return 0;
}
