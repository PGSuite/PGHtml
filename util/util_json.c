#include <stdarg.h>
#include <stdio.h>

#include "utils.h"

int _json_skip_spaces(json *json, int *pos) {
	while(json->source[*pos] && (json->source[*pos]==' ' || json->source[*pos]=='\t' || json->source[*pos]=='\n'))
		if (str_utf8_next(json->source, pos)) return 1;
	return 0;
}

int _json_is_value_char(char c) {
	if (c >= 'a' && c <= 'z') return 1;
	if (c >= 'A' && c <= 'Z') return 1;
	if (c >= '0' && c <= '9') return 1;
	if (c == '.' || c == '_' || c == '#' || c == '-' || c == '$' || c == '@') return 1;
	return 0;
}

int _json_parse_value(json *json, int *pos) {

	int in_quotes = json->source[*pos]=='"';
	if (in_quotes) (*pos)++;

	while(json->source[*pos]) {
		char c = json->source[*pos];
		if (in_quotes && c=='\\') {	(*pos)=(*pos)+2; continue;	}
		if (in_quotes && c=='"') { (*pos)++; break; }
		if (!in_quotes && !_json_is_value_char(c) ) break;
		if (str_utf8_next(json->source, pos)) return 1;
	}

	return 0;

}

int _json_entry_add(json *json, json_entry entry) {
	if (entry.key_begin!=-1 && json->source[entry.key_begin]=='"') {
		entry.key_begin++;
		entry.key_end--;
	}
	if (json->entries_len==json->entries_size) {
		int entries_new_size = json->entries_size*2;
		int size_bytes = entries_new_size*sizeof(json_entry);
		json_entry *entries_new = malloc(size_bytes);
		if (entries_new==NULL)
			return log_error(9, size_bytes);
		for(int i=0; i<json->entries_len; i++) {
			entries_new[i] = json->entries[i];
		}
		free(json->entries);
		json->entries = entries_new;
		json->entries_size = entries_new_size;
	}
	json->entries[json->entries_len] = entry;
	json->entries_len++;
	return 0;
}

int _json_parse_array(json *json, int *pos, int parent) {
	int pos_begin=*pos;
	do {
		if (str_utf8_next(json->source, pos)) return 1;
		if (json->source[*pos]==']') break;
		json_entry json_entry;
		json_entry.parent=parent;
		json_entry.value_type = STRING;
		json_entry.array_index = json->entries[parent].array_size++;
		json_entry.array_size = json_entry.key_begin = json_entry.key_end = -1;
		if(_json_skip_spaces(json, pos)) return 1;
		json_entry.value_begin = *pos;
		if(_json_parse_value(json, pos)) return 1;
		json_entry.value_end = *pos-1;
		if(_json_entry_add(json, json_entry)) return 1;
		if(_json_skip_spaces(json, pos)) return 1;
	} while(json->source[*pos]==',');
	if (json->source[*pos]!=']')
		return log_error(55, "Not found array end (char ']')", pos_begin, json->source);
	if (str_utf8_next(json->source, pos)) return 1;
	return 0;
}


int _json_parse_object(json *json, int *pos, int parent) {
	int pos_begin=*pos;
	if(_json_skip_spaces(json, pos)) return 1;
	if (json->source[*pos]!='{')
		return log_error(55, "Not found object start (char '{')", pos_begin, json->source);
	do {
		if (str_utf8_next(json->source, pos)) return 1;
		json_entry json_entry;
		json_entry.parent=parent;
		json_entry.array_size = json_entry.array_index = json_entry.value_begin = json_entry.value_end = -1;
		if(_json_skip_spaces(json, pos)) return 1;
		json_entry.key_begin = *pos;
		if(_json_parse_value(json, pos)) return 1;
		json_entry.key_end = *pos-1;
		if(_json_skip_spaces(json, pos)) return 1;
		if (json->source[*pos]!=':')
			return log_error(55, "Not found name end (char ':')", pos_begin, json->source);
		if (str_utf8_next(json->source, pos)) return 1;
		if(_json_skip_spaces(json, pos)) return 1;
		if (json->source[*pos]=='{') {
			json_entry.value_type = OBJECT;
			if(_json_entry_add(json, json_entry)) return 1;
			if(_json_parse_object(json, pos, json->entries_len-1)) return 1;
		} else if (json->source[*pos]=='[') {
			json_entry.value_type = ARRAY;
			json_entry.array_size = 0;
			if(_json_entry_add(json, json_entry)) return 1;
			if(_json_parse_array(json, pos, json->entries_len-1)) return 1;
		} else {
			json_entry.value_type = STRING;
			json_entry.value_begin = *pos;
			if(_json_parse_value(json, pos)) return 1;
			json_entry.value_end = *pos-1;
			if(_json_entry_add(json, json_entry)) return 1;
		}
		if(_json_skip_spaces(json, pos)) return 1;
	} while(json->source[*pos]==',');
	if (json->source[*pos]!='}')
		return log_error(55, "Not found object end (char '}')", pos_begin, json->source);
	if (str_utf8_next(json->source, pos)) return 1;
	return 0;
}


int json_init(json *json, char *source) {
	json->source = source;
	json->entries_len = 0;
	json->entries_size = 10;
	int size_bytes = json->entries_size*sizeof(json_entry);
	json->entries = malloc(size_bytes);
	if (json->entries==NULL)
		return log_error(9, size_bytes);
	int pos = 0 ;
	return _json_parse_object(json, &pos, -1);
}

int json_free(json *json) {
	if (json->entries==NULL)
		return log_error(56);
	free(json->entries);
	json->entries = NULL;
	json->entries_len = json->entries_size = -1;
	return 0;
}

int _json_find_entry(json_entry **entry, json *json, int error_on_not_found, enum json_entry_value_type value_type, va_list args) {
	int parent = -1;
	char *key = va_arg(args, char *);
	char *key_next = va_arg(args, char *);
	char path[STR_SIZE] = "$";
	if (str_add(path, sizeof(path), ".", key, NULL)) return 1;
	for(int i=0; i<json->entries_len; i++) {
		if (parent!=json->entries[i].parent) continue;
		int entry_key_size = json->entries[i].key_end-json->entries[i].key_begin+1;
		char *entry_key = json->source+json->entries[i].key_begin;
		int j;
		for(j=0; key[j] && j<entry_key_size; j++)
			if(key[j]!=entry_key[j]) break;
		if (key[j] || j!=entry_key_size) continue;
		if (key_next==NULL) {
			if (json->entries[i].value_type!=value_type) break;
			*entry = &(json->entries[i]);
			return 0;
		} else {
			parent = i;
			key = key_next;
			key_next = va_arg(args, char *);
			if (str_add(path, sizeof(path), ".", key, NULL)) return 1;
		}
	}
	if (error_on_not_found) {
		while( key_next!=NULL ) {
			if (str_add(path, sizeof(path), ".", key_next, NULL)) return 1;
			key_next=va_arg(args, char *);
		}
		return log_error(57, value_type, path, json->source);
	}
	return -1;
}

int json_get_str(char *value, int value_size, json *json, int error_on_not_found, ...) {
	value[0]=0;
	va_list args;
	va_start(args, &error_on_not_found);
	json_entry *entry;
	int find = _json_find_entry(&entry, json, error_on_not_found, STRING, args);
	va_end(args);
	if (find) return find;
	if (str_substr(value, value_size, json->source, entry->value_begin, entry->value_end)) return 1;
	if (str_unescaped(value)) return 1;
	return 0;
}

int json_get_bool(int *value, json *json, int error_on_not_found, ...) {
	va_list args;
	va_start(args, &error_on_not_found);
	json_entry *entry;
	int find = _json_find_entry(&entry, json, error_on_not_found, STRING, args);
	va_end(args);
	if (find) return find;
	*value = (json->source[entry->value_begin]=='t');
	return 0;
}

int json_get_int(int *value, json *json, int error_on_not_found, ...) {
	va_list args;
	va_start(args, &error_on_not_found);
	json_entry *entry;
	int find = _json_find_entry(&entry, json, error_on_not_found, STRING, args);
	va_end(args);
	if (find) return find;
	char value_str[16];
	if (str_substr(value_str, sizeof(value_str), json->source, entry->value_begin, entry->value_end)) return 1;
	*value = atoi(value_str);
	return 0;
}

int json_get_stream(stream *stream, json *json, int error_on_not_found, ...) {
	va_list args;
	va_start(args, &error_on_not_found);
	json_entry *entry;
	int find = _json_find_entry(&entry, json, error_on_not_found, STRING, args);
	va_end(args);
	if (find) return find;
	stream_clear(stream);
	if(stream_add_substr_unescaped(stream, json->source, entry->value_begin, entry->value_end)) return 1;
	return 0;
}

int json_get_array_entry(json_entry **entry_array, json *json, int error_on_not_found, ...) {
	*entry_array = NULL;
	va_list args;
	va_start(args, error_on_not_found);
	int find = _json_find_entry(entry_array, json, error_on_not_found, ARRAY, args);
	va_end(args);
	return find;
}

int json_get_array_stream(stream *stream, json *json, json_entry *entry_array, int index) {
	if (index<0 || index>=entry_array->array_size)
		return log_error(63, entry_array->array_size, index);
	json_entry *entry_element = entry_array+1+index;
	if (entry_element->value_type!=STRING)
		return log_error(64, entry_element->value_type);
	stream_clear(stream);
	if(stream_add_substr_unescaped(stream, json->source, entry_element->value_begin, entry_element->value_end)) return 1;
	return 0;
}

int json_get_array_str(char *str, int str_size, json *json, json_entry *entry_array, int index) {
	if (index<0 || index>=entry_array->array_size)
		return log_error(63, entry_array->array_size, index);
	json_entry *entry_element = entry_array+1+index;
	if (entry_element->value_type!=STRING)
		return log_error(64, entry_element->value_type);
	if(str_substr(str, str_size, json->source, entry_element->value_begin, entry_element->value_end)) return 1;
	if(str_unescaped(str)) return 1;
	return 0;
}

