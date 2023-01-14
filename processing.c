#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include "globals.h"
#include "util/utils.h"

FILE *file_list_updates = NULL;
char file_list_updates_name[STR_SIZE];

int file_list_updates_open(char *value) {
	if (str_copy(file_list_updates_name, sizeof(file_list_updates_name), value)) return 1;
	file_list_updates = fopen(file_list_updates_name,"wb");
	if(!file_list_updates)
		return log_error(8, file_list_updates_name);
	return 0;
}

int file_list_updates_close() {
    if (file_list_updates && fclose(file_list_updates))
    	return log_error(14, file_list_updates_name);
    file_list_updates = NULL;
	return 0;
}

int process_include(stream *file, char *directory_root, char *directory) {
	int pos_begin, pos_path, pos_end;
	do  {
		pos_begin = str_find(file->data, file->len, 0, TAG_PGHTML_INCLUDE_BEGIN, 0);
		if (pos_begin==-1) break;
		pos_path = str_find_char_html(file->data, file->len, pos_begin, '>');
		if (pos_path==-1)
			return log_error(16, ">", pos_path);
		pos_end = str_find(file->data, file->len, pos_path, TAG_PGHTML_INCLUDE_END, 0);
		if (pos_end==-1)
			return log_error(16, TAG_PGHTML_INCLUDE_END, pos_path);
		char tag[STR_SIZE];
		if (str_substr(tag, sizeof(tag), file->data, pos_begin, pos_path))
			return 1;
		str_map vars;
		if (str_tag_attributes(tag, &vars))
			return 1;
		char include_filename[STR_SIZE];
		if (str_substr(include_filename, sizeof(include_filename), file->data, pos_path+1, pos_end-1))
			return 1;
		char include_path[STR_SIZE] = "";
		if (str_add(include_path, sizeof(include_path), directory_root, NULL)) return 1;
		if (include_filename[0]!='/') {
			if (str_add(include_path, sizeof(include_path), directory, include_filename, NULL)) return 1;
		} else {
			if (str_add(include_path, sizeof(include_path), include_filename+1, NULL)) return 1;
		}
		int include_path_len = strlen(include_path);
		for(int i=0; i<include_path_len; i++)
			if (include_path[i]=='/' || include_path[i]=='\\') include_path[i]=FILE_SEPARATOR[0];
		stream include_body;
		if (file_read(include_path, &include_body))
			return 1;
		if (process_vars(&include_body, &vars))
			return 1;
		stream file_new;
		if (stream_init(&file_new))
			return 1;
		if (stream_add_substr(&file_new, file->data, 0, pos_begin-1) ||
			stream_add_substr(&file_new, include_body.data, 0, include_body.len-1) ||
			stream_add_substr(&file_new, file->data, pos_end+strlen(TAG_PGHTML_INCLUDE_END), file->len-1)) {
			stream_free(&file_new);
			return 1;
		};
		if (stream_replace(file, &file_new)) return 1;
	} while(1);
	return 0;
}

int process_vars(stream *file, str_map *vars) {
	stream file_new;
	if (stream_init(&file_new))
		return 1;
	int pos_end=0;
	while(1) {
		int pos_begin = str_find(file->data, file->len, pos_end, "${", 0);
		if (pos_begin==-1) {
			if (stream_add_substr(&file_new, file->data, pos_end, file->len-1)) {
				return 1;
			}
			break;
		}
		if(stream_add_substr(&file_new, file->data, pos_end, pos_begin-1))
			return 1;
		pos_end = str_find(file->data, file->len, pos_begin, "}", 0);
		if (pos_end==-1) {
			stream_free(&file_new);
			return log_error(17, pos_begin);
		}
		pos_end++;
		char var_name[STR_SIZE];
		if (str_substr(var_name, sizeof(var_name), file->data, pos_begin+2, pos_end-2)) {
			stream_free(&file_new);
			return 1;
		}
		char *var_value = NULL;
		for(int i=0; i<vars->len; i++) {
			if (strcmp(var_name,vars->keys[i])==0)
				var_value = vars->values[i];
		}
		if (var_value==NULL) {
			char var_value_not_found[STR_SIZE] = "";
			if (str_add(var_value_not_found, sizeof(var_value_not_found), "${", var_name, "}", NULL)) {
				stream_free(&file_new);
				return 1;
			}
			var_value = var_value_not_found;
		}
		if (stream_add_substr(&file_new, var_value, 0, strlen(var_value)-1))
			return 1;
	}
	if (stream_replace(file, &file_new)) return 1;
	return 0;
}

int process_pg_sql(stream *file) {
	stream file_new;
	if (stream_init(&file_new))
		return 1;
	int pos_end=0;
	while(1) {
		int pos_begin = str_find(file->data, file->len, pos_end, TAG_PGHTML_SQL_BEGIN, 0);
		if (pos_begin==-1) {
			if (stream_add_substr(&file_new, file->data, pos_end, file->len-1))
				return 1;
			break;
		}
		if(stream_add_substr(&file_new, file->data, pos_end, pos_begin-1))
			return 1;
		pos_end = str_find(file->data, file->len, pos_begin, TAG_PGHTML_SQL_END, 0);
		if (pos_end==-1) {
			stream_free(&file_new);
			return log_error(18, TAG_PGHTML_SQL_END, pos_begin);
		}
		pos_begin += strlen(TAG_PGHTML_SQL_BEGIN);
		char query[STR_SIZE];
		if (str_substr(query, sizeof(query), file->data, pos_begin, pos_end-1))
			return 1;
		pos_end += strlen(TAG_PGHTML_SQL_END);
		PGresult *pg_result = PQexec(pg_conn, query);
		if (pg_check_result(pg_result, query, 1)) return 1;
		int row_count = PQntuples(pg_result);
		int column_count = PQnfields(pg_result);
        for(int i=0; i<row_count; i++) {
        	if (i>0 && stream_add_str(&file_new, "\r\n", NULL))
        		return 1;
        	for(int j=0; j<column_count; j++) {
            	if (j>0 && stream_add_substr(&file_new, " ", 0, 0))
            		return 1;
				char *value = PQgetvalue(pg_result, i, j);
				if (stream_add_substr(&file_new, value, 0, strlen(value)-1))
					return 1;
	       	}
        }
        PQclear(pg_result);
	}
	if (stream_replace(file, &file_new)) return 1;
	return 0;
}

int process_file(char *directory_root, char *directory, char *filename, char *extension_source) {
	log_info("processing file %s%s", directory, filename);
	char filepath_source[STR_SIZE] = "";
	if (str_add(filepath_source, sizeof(filepath_source), directory_root, directory, filename, NULL)) return 1;
	char extension_dest[STR_SIZE];
	if (str_substr(extension_dest, sizeof(extension_dest), extension_source, 2, strlen(extension_source)-1)) return 1;
	char filepath_dest[STR_SIZE];
	if (str_substr(filepath_dest, sizeof(filepath_dest), filepath_source, 0, strlen(filepath_source)-strlen(extension_source)-1)) return 1;
	if (str_add(filepath_dest, sizeof(filepath_dest), extension_dest, NULL)) return 1;
	stream file;
	if (file_read(filepath_source, &file))
		return 1;
	str_map vars; str_map_clear(&vars);
	if (str_map_put(&vars, "directory_root",  directory_root))  return 1;
	if (str_map_put(&vars, "directory",       directory))       return 1;
	if (str_map_put(&vars, "filename",        filename))        return 1;
	if (str_map_put(&vars, "filepath_source", filepath_source)) return 1;
	if (str_map_put(&vars, "filepath_dest",   filepath_dest))   return 1;
	//
	if (process_include(&file, directory_root, directory) || process_vars(&file, &g_vars) || process_vars(&file, &vars) || process_pg_sql(&file)) {
		stream_free(&file);
		return 1;
	}
	int compare = file_compare(filepath_dest, &file);
	if (compare>0) {
		stream_free(&file);
		return 1;
	}
	if (compare<0 && file_write(filepath_dest, &file)) {
		stream_free(&file);
		return 1;
	}
	log_info("  done, %s", compare==0 ? "no update required" : compare==-1 ? "file created" : "file updated");
	if (compare) {
		char filename_dest[STR_SIZE];
		if (str_substr(filename_dest, sizeof(filename_dest), filename, 0, strlen(filename)-strlen(extension_source)-1) || str_add(filename_dest, sizeof(filename_dest), extension_dest, NULL)) {
			stream_free(&file);
			return 1;
		}
		if (file_list_updates && fprintf(file_list_updates, "%s%s\n", directory, filename_dest)<=0) {
			fclose(file_list_updates);
			file_list_updates = NULL;
			return log_error(12, file_list_updates_name);
		}
	}
	stream_free(&file);
	return 0;
}

int process_directory(char *directory_root, char *directory) {
	int result = 0;
	char dir_path[STR_SIZE] = "";
	if(str_add(dir_path, sizeof(dir_path), directory_root, directory, NULL)) return 1;
    DIR *dir = opendir(dir_path);
    struct dirent *dir_ent;
    while ((dir_ent = readdir(dir)) != NULL) {
    	if (!strcmp(dir_ent->d_name, ".") || !strcmp(dir_ent->d_name, "..")) continue;
		char path_ent[STR_SIZE] = "";
		if (str_add(path_ent, sizeof(path_ent), directory_root, directory, dir_ent->d_name, NULL)) return 1;
		int is_dir;
		if (file_is_dir(path_ent, &is_dir)) return 1;
		if (is_dir) {
			char directory_ent[STR_SIZE] = "";
			if (str_add(directory_ent, sizeof(path_ent), directory, dir_ent->d_name, FILE_SEPARATOR, NULL)) return 1;
			if (process_directory(directory_root, directory_ent))
				result = 1;
			continue;
		}
		char extension[STR_SIZE];
		if (file_extension(extension, sizeof(extension), dir_ent->d_name )) return 1;
		if (extension[0]==0) continue;
		int i;
		for (i=0;i<file_extensions.len; i++)
			if (!strcmp(extension,file_extensions.values[i])) break;
		if (i==file_extensions.len) continue;
		if (process_file(directory_root, directory, dir_ent->d_name, extension))
			result = 1;
    }
    closedir(dir);
    return result;
}

int process_directories() {
	int result = 0;
	char directory_root[STR_SIZE];
    for(int i=0; i<directories.len; i++) {
    	int is_dir;
    	if (file_is_dir(directories.values[i], &is_dir)) continue;
    	if (is_dir) {
    		if (str_copy(directory_root, sizeof(directory_root), directories.values[i])) return 1;
    		char c = directory_root[strlen(directory_root)-1];
    		if (c!='/' && c!='\\')
    			if (str_add(directory_root, sizeof(directory_root), FILE_SEPARATOR, NULL)) return 1;
    		log_info("directory %s", directory_root);
    		int result_directory = process_directory(directory_root, "");
    		result = result || result_directory;
    		log_info("directory processed %s", result_directory ? "with error(s)" : "successfully");
    	} else {
    		char filename[STR_SIZE];
    		char extension[STR_SIZE];
    		if (file_filename(filename, sizeof(filename), directories.values[i])) return 1;
    		if (file_extension(extension, sizeof(extension), directories.values[i])) return 1;
    		if (strlen(extension)<3 || extension[0]!='p' || extension[1]!='g')
    			return log_error(22, extension);
    		if (str_substr(directory_root, sizeof(directory_root), directories.values[i], 0, strlen(directories.values[i])-1)) return 1;
    		if (directory_root[0]==0 && str_copy(directory_root, sizeof(directory_root), ".")) return 1;
    		result = process_file(directory_root, "", filename, extension) || result;
    	}
    }
	return result;
}
