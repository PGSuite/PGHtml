#include "util/utils.h"

int vars_tag_attributes(char *tag, str_map *attributes) {
	int pos_begin,pos_end;
	int tag_len = strlen(tag);
	int tag_attributes_size_max = sizeof(attributes->keys)/sizeof(attributes->keys[0]);
	attributes->len = 0;
	for(int pos=0; pos<tag_len; pos++) {
		if (tag[pos]=='=') {
			if (attributes->len==tag_attributes_size_max)
				return log_error(6, tag);
			for(pos_end = pos-1; pos_end>0 && (tag[pos_end]==' ' || tag[pos_end]=='\t');  pos_end--);
			for(pos_begin = pos_end-1; pos_begin>0 && tag[pos_begin]!=' ' && tag[pos_begin]!='\t';  pos_begin--);
			if (pos_begin==0 || pos_end==0)
				return log_error(7, pos, tag);
			pos_begin++;
			if (str_substr(attributes->keys[attributes->len], sizeof(attributes->keys[0]), tag, pos_begin, pos_end))
				return 1;
			for(pos_begin = pos+1; pos_begin<tag_len && (tag[pos_begin]==' ' || tag[pos_begin]=='\t');  pos_begin++);
			if (pos_begin==tag_len || (tag[pos_begin]!='"' && tag[pos_begin]!='\'')  )
				return log_error(7, pos, tag);
			for(pos_end = pos_begin+1; pos_end<tag_len && tag[pos_end]!=tag[pos_begin]; pos_end++);
			if (pos_end==tag_len)
				return log_error(7, pos, tag);
			if (str_substr(attributes->values[attributes->len], sizeof(attributes->values[0]), tag, pos_begin+1, pos_end-1))
				return 1;
			attributes->len++;
		}
	}
	return 0;
}
