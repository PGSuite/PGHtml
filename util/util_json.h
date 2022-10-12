#ifndef UTIL_JSON_H_
#define UTIL_JSON_H_

typedef enum json_entry_type {
	OBJECT,
	ARRAY,
	VALUE
};



typedef struct
{

	enum json_entry_type type;

	int parent;
	int first;
	int next;

	int key_begin;
	int key_end;

	int value_begin;
	int value_end;

} json_entry;


typedef struct
{

	json_entry *entries;
	int entries_size;
	int entries_size_max;

	char *source;

} json;

#endif /* UTIL_JSON_H_ */
