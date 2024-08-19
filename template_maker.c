#include "globals.h"
#include "util/utils.h"

int template_maker_tag_skip_spaces(char *html, int *pos) {
	int p = *pos;
	for(; html[p] && html[p]!='>' && (html[p]==' ' || html[p]=='\t'); p++);
	if (!html[p]) return log_error(17, '>', *pos);
	*pos = p;
	return 0;
}

int template_maker_tag_skip_chars(char *html, int *pos, char stop_char) {
	int p = *pos;
	for(; html[p] && html[p]!='>' && html[p]!=' ' && html[p]!='\t' && html[p]!=stop_char; p++);
	if (!html[p]) return log_error(17, '>', *pos);
	if (stop_char!=0 && html[p]=='>')	return log_error(17, '>', *pos);
	*pos = p;
	return 0;
}

int template_maker_html_parse_element(char *html, int pos_begin, int *pos_end, char *tag, int tag_size, str_map *attributes, stream *body) {
	int pos=pos_begin;
	if (template_maker_tag_skip_chars(html, &pos, 0)) return 1;
	if (str_substr(tag, tag_size, html, pos_begin+1, pos-1)) return 1;
	str_map_clear(attributes);
	while(1) {
		if(template_maker_tag_skip_spaces(html, &pos)) return 1;
		if (html[pos]=='>') break;
		int name_begin = pos;
		if (template_maker_tag_skip_chars(html, &pos, '=')) return 1;
		int name_end = pos-1;
		if (html[pos]!='=') {
			if (template_maker_tag_skip_spaces(html, &pos)) return 1;
			if (html[pos]!='=') return log_error(7, name_end);
		}
		pos++;
		char name[STR_SIZE];
		if (str_substr(name, sizeof(name), html, name_begin, name_end)) return 1;
		if (template_maker_tag_skip_spaces(html, &pos)) return 1;
		if (html[pos]=='>')	return log_error(7, name_end);
		int value_begin = pos;
		if (html[value_begin]!='"' && html[value_begin]!='\'') return log_error(7, value_begin);
		for(pos++; html[pos] && html[pos]!=html[value_begin];pos++);
		if (!html[pos]) return log_error(7, value_begin);
		int value_end = pos++;
		if ( (value_begin==value_end) || (html[value_end]!='"'  && html[value_end]!='\'') ) return log_error(7, value_begin);
		char value[STR_SIZE] = "";
		if (value_begin+1<value_end && str_substr(value, sizeof(value), html, value_begin+1, value_end-1)) return 1;
		str_map_put(attributes, name, value);
	}
	int body_begin = ++pos;
	char tag_end_name[STR_SIZE] = "";
	if (str_add(tag_end_name, sizeof(tag_end_name), "</", tag, ">", NULL)) return 1;
	int tag_end_pos = str_find(html, pos, tag_end_name, 0);
	if (tag_end_pos==-1) return log_error(18, tag_end_name, pos);
	stream_clear(body);
	if (stream_add_substr(body, html, body_begin, tag_end_pos-1)) return 1;
	*pos_end = tag_end_pos+strlen(tag_end_name)-1;
	return 0;
}

int template_maker_process(PGconn *pg_conn, char *directory, char *filename) {
	char extension[STR_SIZE];
	if (file_extension(extension, sizeof(extension), filename)) return 1;
	if (strlen(extension)<3 || extension[0]!='p' || extension[1]!='g') return 0;
	log_info("processing %s%s template", directory, filename);
	char filename_dest[STR_SIZE];
	if (str_substr(filename_dest, sizeof(filename_dest), filename, 0, strlen(filename)-strlen(extension)-1)) return 1;
	if (str_add(filename_dest, sizeof(filename_dest), extension+2, NULL)) return 1;
	char filepath_source[STR_SIZE] = "";
	if (str_add(filepath_source, sizeof(filepath_source), http_directory, FILE_SEPARATOR, directory, filename, NULL)) return 1;
	char filepath_dest[STR_SIZE] ="";
	if (str_add(filepath_dest, sizeof(filepath_dest), http_directory, FILE_SEPARATOR, directory, filename_dest, NULL)) return 1;
	stream_list vars;
	stream_list_init(&vars);
	if (stream_list_add_str(&vars, "http_directory",  http_directory))  return 1;
	if (stream_list_add_str(&vars, "directory",       directory))       return 1;
	if (stream_list_add_str(&vars, "filename",        filename))        return 1;
	if (stream_list_add_str(&vars, "filepath_source", filepath_source)) return 1;
	if (stream_list_add_str(&vars, "filepath_dest",   filepath_dest))   return 1;
	//
	stream file;
	if (file_read(filepath_source, &file))
		return 1;
	if (template_maker_include(&file, directory)) {
		stream_list_free(&vars);
		stream_free(&file);
		return 1;
	}
	int res = template_maker_vars(&file, &vars);
	stream_list_free(&vars);
	if (res) {
		stream_free(&file);
		return 1;
	}
	if (template_maker_sql(pg_conn, &file)) {
		stream_free(&file);
		return 1;
	}
	int changed = file_write_if_changed(filepath_dest, &file);
	stream_free(&file);
	if (changed>0) return 1;
	log_info("file %s%s %s", directory, filename_dest, changed==-1 ? "created" : changed==-2 ? "updated" : "does not update required");
	return 0;
}

int template_maker_include(stream *file, char *directory) {
	int pos_begin, pos_path, pos_end;
	do  {
		pos_begin = str_find(file->data, 0, "<" TAG_PGHTML_INCLUDE, 0);
		if (pos_begin==-1) break;
		char tag[STR_SIZE];
		str_map attributes;
		stream include_filename;
		if (stream_init(&include_filename)) return 1;
		if (template_maker_html_parse_element(file->data, pos_begin, &pos_end, tag, sizeof(tag), &attributes, &include_filename)) {
			stream_free(&include_filename);
			return 1;
		}
		char include_path[STR_SIZE] = "";
		int res = str_add(include_path, sizeof(include_path), http_directory, FILE_SEPARATOR, include_filename.data[0]!='/' ? directory : "", include_filename.data, NULL);
		stream_free(&include_filename);
		if (res) return 1;
		#ifdef _WIN32
			str_replace_char(include_path, '/', FILE_SEPARATOR[0]);
		#else
			str_replace_char(include_path, '\\', FILE_SEPARATOR[0]);
		#endif
		stream include_body;
		if (file_read(include_path, &include_body))
			return 1;
		stream file_new;
		if (stream_init(&file_new))
			return 1;
		if (pos_begin>0 && stream_add_substr(&file_new, file->data, 0, pos_begin-1)) {
			stream_free(&file_new);
			return 1;
		};
		stream_list vars;
		stream_list_init(&vars);
		for(int i=0; i<attributes.len; i++)
			if (stream_list_add_str(&vars, attributes.keys[i], attributes.values[i])) {
				stream_list_free(&vars);
				stream_free(&file_new);
				return 1;
			}
		res = 0;
		for(int pos=0; pos<include_body.len; pos++) {
			if (include_body.data[pos]=='$' && include_body.data[pos+1]=='{') {
				if (template_maker_var_replace(&file_new, &include_body, &pos, &vars)>0) { res=1; break; }
				continue;
			}
			if (stream_add_char(&file_new, include_body.data[pos])) { res=1; break; }
		}
		stream_list_free(&vars);
		if (res) {
			stream_free(&file_new);
			return 1;
		};
		if (pos_end<file->len-1 && stream_add_substr(&file_new, file->data, pos_end+1, file->len-1)) {
			stream_free(&file_new);
			return 1;
		};
		stream_replace(file, &file_new);
	} while(1);
	return 0;
}

int template_maker_var_replace(stream *file_dest, stream *file_source, int *pos, stream_list *vars) {
	int pos_end = str_find(file_source->data, *pos, "}", 0);
	if (pos_end==-1) return log_error(17, '}', *pos);
	int var_len = pos_end-*pos-2;
	int var_index;
	for(var_index=0; var_index<vars->len; var_index++) {
		if (strlen(vars->names[var_index])!=var_len) continue;
		if (strncmp(vars->names[var_index], file_source->data+*pos+2, var_len)) continue;
		break;
	}
	int res;
	if (var_index<vars->len) {
		if (stream_add_str(file_dest, vars->streams[var_index].data, NULL)) return 1;
		res = 0;
	} else {
		if (stream_add_substr(file_dest, file_source->data, *pos, pos_end)) return 1;
		res = -1;
	}
	*pos = pos_end;
	return res;
}


int template_maker_vars(stream *file, stream_list *vars) {
	int pos_begin=0, pos_end;
	do  {
		pos_begin = str_find(file->data, pos_begin, "<" TAG_PGHTML_VAR, 0);
		if (pos_begin==-1) break;
		char tag[STR_SIZE];
		str_map attributes;
		stream value;
		if (stream_init(&value)) return 1;
		if (template_maker_html_parse_element(file->data, pos_begin, &pos_end, tag, sizeof(tag), &attributes, &value)) {
			stream_free(&value);
			return 1;
		}
		if (strcmp(tag, TAG_PGHTML_VAR)) {
			stream_free(&value);
			return log_error(83, tag);
		}
		int attribute_name_index = str_map_index(&attributes, "name");
		if (attribute_name_index==-1) {
			stream_free(&value);
			return log_error(83, tag);
		}
		int res = stream_list_add_str(vars, attributes.values[attribute_name_index], value.data);
		stream_free(&value);
		if(res)
			return 1;
		stream file_new;
		if (stream_init(&file_new))
			return 1;
		if (pos_begin>0 && stream_add_substr(&file_new, file->data, 0, pos_begin-1)) {
			stream_free(&file_new);
			return 1;
		}
		if (file->data[pos_end+1]=='\r' && file->data[pos_end+2]=='\n')
			pos_end = pos_end + 2;
		else if (file->data[pos_end+1]=='\r')
			pos_end = pos_end + 1;
		if (pos_end<file->len-1 && stream_add_substr(&file_new, file->data, pos_end+1, file->len-1)) {
			stream_free(&file_new);
			return 1;
		}
		stream_replace(file, &file_new);
	} while(1);
	pos_begin=0;
	do  {
		pos_begin = pos_end = str_find(file->data, pos_begin, "${", 0);
		if (pos_begin==-1) break;
		stream file_new;
		if (stream_init(&file_new))
			return 1;
		if (pos_begin>0 && stream_add_substr(&file_new, file->data, 0, pos_begin-1)) {
			stream_free(&file_new);
			return 1;
		}
		int res = template_maker_var_replace(&file_new, file, &pos_end, vars);
		if (res>0) {
			stream_free(&file_new);
			return 1;
		}
		if (res==-1) pos_begin++;
		if (pos_end<file->len-1 && stream_add_substr(&file_new, file->data, pos_end+1, file->len-1)) {
			stream_free(&file_new);
			return 1;
		};
		stream_replace(file, &file_new);
	} while(1);
	return 0;
}

int template_maker_sql(PGconn *pg_conn, stream *file) {
	int pos_begin, pos_end;
	do  {
		pos_begin = str_find(file->data, 0, "$$", 0);
		if (pos_begin==-1) break;
		pos_end = str_find(file->data, pos_begin+2, "$$", 0);
		if (pos_end==-1) break;
		stream sql_query;
		if (stream_init(&sql_query)) return 1;
		if (stream_add_substr(&sql_query, file->data, pos_begin+2, pos_end-1)) return 1;
		PGresult *pg_result;
		if (pg_select(&pg_result, pg_conn, 1, sql_query.data, 0)) {
			stream_free(&sql_query);
			return 1;
		}
		stream file_new;
		if (stream_init(&file_new))
			return 1;
		if (pos_begin>0 && stream_add_substr(&file_new, file->data, 0, pos_begin-1)) {
			stream_free(&file_new);
			return 1;
		};
		int row_count = PQntuples(pg_result);
		int column_count = PQnfields(pg_result);
		int res = 0;
        for(int r=0; r<row_count; r++) {
        	if (r>0 && stream_add_str(&file_new, "\r\n", NULL)) { res=1; break; }
        	for(int c=0; c<column_count; c++) {
            	if (c>0 && stream_add_str(&file_new, " ", NULL)) { res=1; break; }
				char *value = PQgetisnull(pg_result, r, c) ? "<null>" : PQgetvalue(pg_result, r, c);
				if (stream_add_str(&file_new, value, NULL)) { res=1; break; }
	       	}
        	if (res) break;
        }
        PQclear(pg_result);
		stream_free(&sql_query);
		if (res) {
			stream_free(&file_new);
			return 1;
		}
		if (pos_end<file->len-1 && stream_add_substr(&file_new, file->data, pos_end+2, file->len-1)) {
			stream_free(&file_new);
			return 1;
		};
		stream_replace(file, &file_new);
	} while(1);
	return 0;
}
