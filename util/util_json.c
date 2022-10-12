#include <stdarg.h>

#include "util_str.h"
#include "util_json.h"

int _json_skip_spaces(json *json, int *pos) {

	while(json->source[*pos] && (json->source[*pos]==' ' || json->source[*pos]=='\t' || json->source[*pos]=='\n'))
		if (str_utf8_next(json->source, pos)) return 1;

	return 0;

}

int _json_is_value_char(char c) {
	if (c >= 'a' && c <= 'z') return 1;
	if (c >= 'A' && c <= 'Z') return 1;
	if (c >= '0' && c <= '9') return 1;
	if (c == '_' && c == '#' && c == '-' && c == '$' && c == '@') return 1;
	return 0;
}

int _json_skip_value(json *json, int *pos) {

	int in_quotes = json->source[*pos]=='"';
	if (in_quotes) (*pos)++;

	while(json->source[*pos]) {
		char c = json->source[*pos];
		if (in_quotes && c=='"') { (*pos)++; break; }
		if (!in_quotes && !_json_is_value_char(c) ) break;
		if (str_utf8_next(json->source, pos)) return 1;
	}

	return 0;

}


int _json_entry_add(json *json, json_entry entry) {
	if (json->entries_size==json->entries_size_max) {
		int entries_new_size_max = json->entries_size_max*2;
		int size_bytes = entries_new_size_max*sizeof(json_entry);
		json_entry *entries_new = malloc(size_bytes);
		if (entries_new==NULL) {
			log_stderr_print(9, size_bytes);
			return 1;
		}
		for(int i=0; i<json->entries_size; i++) {
			entries_new[i] = json->entries[i];
		}
		free(json->entries);
		json->entries = entries_new;
		json->entries_size_max = entries_new_size_max;
	}
	json->entries[json->entries_size] = entry;
	json->entries_size++;
	printf("*** _json_entry_add %d\n", json->entries_size);
	return 0;
}


int _json_parse_object(json *json, int pos_begin) {

	int pos=pos_begin;
	if(_json_skip_spaces(json, &pos)) return 1;
	if (json->source[pos]!='{') {
		log_stderr_print(55, "Not found bracket '{'", pos_begin, json->source);
		return 1;
	}
	do {

		pos++;

		json_entry json_entry;
		json_entry.key_begin=-1;
		json_entry.key_end=-1;
		if(_json_skip_spaces(json, &pos)) return 1;
		json_entry.value_begin = pos;
		if(_json_skip_value(json, &pos)) return 1;
		json_entry.value_end = pos-1;
		if(_json_skip_spaces(json, &pos)) return 1;

		printf("*** 1 %d %c\n", pos, json->source[pos]);

		if (json->source[pos]==':') {
			pos++;
			json_entry.key_begin=json_entry.value_begin;
			json_entry.key_end=json_entry.value_end;
			if(_json_skip_spaces(json, &pos)) return 1;
			json_entry.value_begin = pos;
			if(_json_skip_value(json, &pos)) return 1;

			json_entry.value_end = pos-1;
			if(_json_skip_spaces(json, &pos)) return 1;
		}

		if(_json_entry_add(json, json_entry)) return 1;

		printf("*** 5 %d %c\n", pos, json->source[pos]);

	} while(json->source[pos]==',');

	printf("*** 9 %d %c\n", pos, json->source[pos]);

	if (json->source[pos]!='}') {
		log_stderr_print(55, "Not found bracket '}'", pos_begin, json->source);
		return 1;
	}

	return 0;

}


int json_init(json *json, char *source) {

	json->source = source;

	json->entries_size = 0;
	json->entries_size_max = 10;
	int size_bytes = json->entries_size_max*sizeof(json_entry);
	json->entries = malloc(size_bytes);
	if (json->entries==NULL) {
		log_stderr_print(9, size_bytes);
		return 1;
	}
	return _json_parse_object(json, 0);

}

int json_get_value_str(char *dest, int dest_size, json *json, ...) {

		va_list args;
		va_start(args, json);

		char *key = va_arg(args, char *);

		printf("*** key = %s %d\n", key, json->entries_size);

		for(int i=0; i<json->entries_size; i++) {
			int entry_key_size = json->entries->key_end-json->entries->key_begin+1;
			char *entry_key = json->source+json->entries->key_begin;
			printf("*** %d> %d %c%c", i, entry_key_size, entry_key[0], entry_key[1]);
			int j;
			for(j=0; key[j] && j<entry_key_size; j++)
				if(key[j]!=entry_key[j]) break;
			if (key[j] || j!=entry_key_size) continue;
			// str_substr(dest, dest_size, json->source, json ->key_begin, len);

		}

		return -1;



		/*
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
		*/

		va_end(args);


}



