#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include "globals.h"
#include "util/util_file.h"

FILE *file_list_updates = NULL;
char file_list_updates_name[STR_SIZE_MAX];

int file_list_updates_open(char *value) {
	if (str_copy(file_list_updates_name, sizeof(file_list_updates_name), value)) return 1;
	file_list_updates = fopen(file_list_updates_name,"wb");
	if(!file_list_updates) {
		log_stderr_print(8, file_list_updates_name);
	    return 1;
	}
	return 0;
}

int file_list_updates_close() {
    if (file_list_updates && fclose(file_list_updates)) {
		log_stderr_print(14, file_list_updates_name);
		return 1;
	}
    file_list_updates = NULL;
	return 0;
}


int process_include(FILE_BODY *file, char *directory_root, char *directory) {
	int pos_begin, pos_path, pos_end;
	do  {
		pos_begin = str_find(file->body, file->size, 0, TAG_PGHTML_INCLUDE_BEGIN, 0);
		if (pos_begin==-1) break;
		pos_path = str_find_char_html(file->body, file->size, pos_begin, '>');
		if (pos_path==-1) {
			log_stderr_print(16, ">", pos_path);
			return 1;
		}
		pos_end = str_find(file->body, file->size, pos_path, TAG_PGHTML_INCLUDE_END, 0);
		if (pos_end==-1) {
			log_stderr_print(16, TAG_PGHTML_INCLUDE_END, pos_path);
			return 1;
		}
		char tag[STR_SIZE_MAX];
		if (str_substr(tag, sizeof(tag), file->body, pos_begin, pos_path))
			return 1;
		STR_MAP vars;
		if (str_tag_attributes(tag, &vars))
			return 1;
		char include_filename[STR_SIZE_MAX];
		if (str_substr(include_filename, sizeof(include_filename), file->body, pos_path+1, pos_end-1))
			return 1;
		char include_path[STR_SIZE_MAX] = "";
		if (str_add(include_path, sizeof(include_path), directory_root, NULL)) return 1;
		if (include_filename[0]!='/') {
			if (str_add(include_path, sizeof(include_path), directory, include_filename, NULL)) return 1;
		} else {
			if (str_add(include_path, sizeof(include_path), include_filename+1, NULL)) return 1;
		}
		int include_path_len = strlen(include_path);
		for(int i=0; i<include_path_len; i++)
			if (include_path[i]=='/' || include_path[i]=='\\') include_path[i]=FILE_SEPARATOR[0];
		FILE_BODY include_body;
		if (file_read(include_path, &include_body))
			return 1;
		if (process_vars(&include_body, &vars))
			return 1;
		FILE_BODY file_new;
		if (file_body_init(&file_new))
			return 1;
		if (file_body_add_substr(&file_new, file->body, 0, pos_begin-1) ||
			file_body_add_substr(&file_new, include_body.body, 0, include_body.size-1) ||
			file_body_add_substr(&file_new, file->body, pos_end+strlen(TAG_PGHTML_INCLUDE_END), file->size-1)) {
			file_body_free(&file_new);
			return 1;
		};
		if (file_body_replace(file, &file_new)) return 1;
	} while(1);
	return 0;
}

int process_vars(FILE_BODY *file, STR_MAP *vars) {
	FILE_BODY file_new;
	if (file_body_init(&file_new))
		return 1;
	int pos_end=0;
	while(1) {
		int pos_begin = str_find(file->body, file->size, pos_end, "${", 0);
		if (pos_begin==-1) {
			if (file_body_add_substr(&file_new, file->body, pos_end, file->size-1)) {
				return 1;
			}
			break;
		}
		if(file_body_add_substr(&file_new, file->body , pos_end, pos_begin-1))
			return 1;
		pos_end = str_find(file->body, file->size, pos_begin, "}", 0);
		if (pos_end==-1) {
			log_stderr_print(17, pos_begin);
			file_body_free(&file_new);
			return 1;
		}
		pos_end++;
		char var_name[STR_SIZE_MAX];
		if (str_substr(var_name, sizeof(var_name), file->body, pos_begin+2, pos_end-2)) {
			file_body_free(&file_new);
			return 1;
		}
		char *var_value = NULL;
		for(int i=0; i<vars->size; i++) {
			if (strcmp(var_name,vars->keys[i])==0)
				var_value = vars->values[i];
		}
		if (var_value==NULL) {
			char var_value_not_found[STR_SIZE_MAX] = "";
			if (str_add(var_value_not_found, sizeof(var_value_not_found), "${", var_name, "}", NULL)) {
				file_body_free(&file_new);
				return 1;
			}
			var_value = var_value_not_found;
		}
		if (file_body_add_substr(&file_new, var_value, 0, strlen(var_value)-1))
			return 1;
	}
	if (file_body_replace(file, &file_new)) return 1;
	return 0;
}

int process_pg_sql(FILE_BODY *file) {
	FILE_BODY file_new;
	if (file_body_init(&file_new))
		return 1;
	int pos_end=0;
	while(1) {
		int pos_begin = str_find(file->body, file->size, pos_end, TAG_PGHTML_SQL_BEGIN, 0);
		if (pos_begin==-1) {
			if (file_body_add_substr(&file_new, file->body , pos_end, file->size-1))
				return 1;
			break;
		}
		if(file_body_add_substr(&file_new, file->body , pos_end, pos_begin-1))
			return 1;
		pos_end = str_find(file->body, file->size, pos_begin, TAG_PGHTML_SQL_END, 0);
		if (pos_end==-1) {
			log_stderr_print(18, TAG_PGHTML_SQL_END, pos_begin);
			file_body_free(&file_new);
			return 1;
		}
		pos_begin += strlen(TAG_PGHTML_SQL_BEGIN);
		char query[STR_SIZE_MAX];
		if (str_substr(query, sizeof(query), file->body, pos_begin, pos_end-1))
			return 1;
		pos_end += strlen(TAG_PGHTML_SQL_END);
		PGresult *pg_result = PQexec(pg_conn, query);
	    if(PQresultStatus(pg_result) != PGRES_TUPLES_OK) {
	    	log_stderr_print(19, query, PQerrorMessage(pg_conn));
        	return 1;
        }
		int row_count = PQntuples(pg_result);
		int column_count = PQnfields(pg_result);
        for(int i=0; i<row_count; i++) {
        	if (i>0 && file_body_add_str(&file_new, "\r\n", NULL))
        		return 1;
        	for(int j=0; j<column_count; j++) {
            	if (j>0 && file_body_add_substr(&file_new, " ", 0, 0))
            		return 1;
				char *value = PQgetvalue(pg_result, i, j);
				if (file_body_add_substr(&file_new, value, 0, strlen(value)-1))
					return 1;
	       	}
        }
        PQclear(pg_result);
	}
	if (file_body_replace(file, &file_new)) return 1;
	return 0;
}

int process_file(char *directory_root, char *directory, char *filename, char *extension_source) {
	char filepath_source[STR_SIZE_MAX] = "";
	if (str_add(filepath_source, sizeof(filepath_source), directory_root, directory, filename, NULL)) return 1;
	char s_out[STR_SIZE_MAX] = "";
	if (str_add(s_out, sizeof(s_out), directory, filename, "...", NULL)) return 1;
	if (str_rtrim(s_out, sizeof(s_out), 60)) return 1;
	log_stdout_println("  file %s", s_out);
	char extension_dest[STR_SIZE_MAX];
	if (str_substr(extension_dest, sizeof(extension_dest), extension_source, 2, strlen(extension_source)-1)) return 1;
	char filepath_dest[STR_SIZE_MAX];
	if (str_substr(filepath_dest, sizeof(filepath_dest), filepath_source, 0, strlen(filepath_source)-strlen(extension_source)-1)) return 1;
	if (str_add(filepath_dest, sizeof(filepath_dest), extension_dest, NULL)) return 1;
	FILE_BODY file;
	if (file_read(filepath_source, &file))
		return 1;
	STR_MAP vars; str_map_clear(&vars);
	if (str_map_put(&vars, "directory_root",  directory_root))  return 1;
	if (str_map_put(&vars, "directory",       directory))       return 1;
	if (str_map_put(&vars, "filename",        filename))        return 1;
	if (str_map_put(&vars, "filepath_source", filepath_source)) return 1;
	if (str_map_put(&vars, "filepath_dest",   filepath_dest))   return 1;
	//
	if (process_include(&file, directory_root, directory) || process_vars(&file, &g_vars) || process_vars(&file, &vars) || process_pg_sql(&file)) {
		file_body_free(&file);
		return 1;
	}
	int compare = file_compare(filepath_dest, &file);
	if (compare>0) {
		file_body_free(&file);
		return 1;
	}
	if (compare<0 && file_write(filepath_dest, &file)) {
		file_body_free(&file);
		return 1;
	}
	log_stdout_printf(" done, %s", compare==0 ? "no update required" : compare==-1 ? "created" : "updated");
	if (compare) {
		char filename_dest[STR_SIZE_MAX];
		if (str_substr(filename_dest, sizeof(filename_dest), filename, 0, strlen(filename)-strlen(extension_source)-1) || str_add(filename_dest, sizeof(filename_dest), extension_dest, NULL)) {
			file_body_free(&file);
			return 1;
		}
		if (file_list_updates && fprintf(file_list_updates, "%s%s\n", directory, filename_dest)<=0) {
			log_stderr_print(12, file_list_updates_name);
			fclose(file_list_updates);
			file_list_updates = NULL;
			return 1;
		}
	}
	file_body_free(&file);
	return 0;
}

int process_directory(char *directory_root, char *directory) {
	int result = 0;
	char dir_path[STR_SIZE_MAX] = "";
	if(str_add(dir_path, sizeof(dir_path), directory_root, directory, NULL)) return 1;
    DIR *dir = opendir(dir_path);
    struct dirent *dir_ent;
    while ((dir_ent = readdir(dir)) != NULL) {
    	if (!strcmp(dir_ent->d_name, ".") || !strcmp(dir_ent->d_name, "..")) continue;
		char path_ent[STR_SIZE_MAX] = "";
		if (str_add(path_ent, sizeof(path_ent), directory_root, directory, dir_ent->d_name, NULL)) return 1;
		int is_dir;
		if (file_is_dir(path_ent, &is_dir)) return 1;
		if (is_dir) {
			char directory_ent[STR_SIZE_MAX] = "";
			if (str_add(directory_ent, sizeof(path_ent), directory, dir_ent->d_name, FILE_SEPARATOR, NULL)) return 1;
			if (process_directory(directory_root, directory_ent))
				result = 1;
			continue;
		}
		char extension[STR_SIZE_MAX];
		if (file_extension(extension, sizeof(extension), dir_ent->d_name )) return 1;
		if (extension[0]==0) continue;
		int i;
		for (i=0;i<file_extensions.size; i++)
			if (!strcmp(extension,file_extensions.values[i])) break;
		if (i==file_extensions.size) continue;
		if (process_file(directory_root, directory, dir_ent->d_name, extension))
			result = 1;
    }
    closedir(dir);
    return result;
}

int process_directories() {
	int result = 0;
	char directory_root[STR_SIZE_MAX];
    for(int i=0; i<directories.size; i++) {
    	int is_dir;
    	if (file_is_dir(directories.values[i], &is_dir)) continue;
    	if (is_dir) {
    		if (str_copy(directory_root, sizeof(directory_root), directories.values[i])) return 1;
    		char c = directory_root[strlen(directory_root)-1];
    		if (c!='/' && c!='\\')
    			if (str_add(directory_root, sizeof(directory_root), FILE_SEPARATOR, NULL)) return 1;
    		log_stdout_println("processing directory %s", directory_root);
    		int result_directory = process_directory(directory_root, "");
    		result = result || result_directory;
    		log_stdout_println("directory processed %s", result_directory ? "with error(s)" : "successfully");
    	} else {
    		log_stdout_println("processing file %s", directories.values[i]);
    		char filename[STR_SIZE_MAX];
    		char extension[STR_SIZE_MAX];
    		if (file_filename(filename, sizeof(filename), directories.values[i])) return 1;
    		if (file_extension(extension, sizeof(extension), directories.values[i])) return 1;
    		if (strlen(extension)<3 || extension[0]!='p' || extension[1]!='g') {
    			log_stderr_print(22, extension);
    			return 1;
    		}
    		if (str_substr(directory_root, sizeof(directory_root), directories.values[i], 0, strlen(directories.values[i])-1)) return 1;
    		if (directory_root[0]==0 && str_copy(directory_root, sizeof(directory_root), ".")) return 1;
    		int result_file = process_file(directory_root, "", filename, extension);
    		result = result || result_file;
    		log_stdout_println("file processed %s", result_file ? "with error(s)" : "successfully");
    	}
    }
	return result;
}
