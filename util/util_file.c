#include "util_file.h"

#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "util_str.h"

#define FILE_BODY_SIZE_INIT 1*1024*1024
#define FILE_BODY_SIZE_STEP 8*1024*1024

int file_body_init(FILE_BODY *file_body) {
	file_body->size_max = FILE_BODY_SIZE_INIT;
	file_body->body = malloc(file_body->size_max);
	if (file_body->body==NULL) {
		log_stderr_print(9, file_body->size_max);
		return 1;
	}
	file_body->size = 0;
	return 0;
}

int file_body_free(FILE_BODY *file_body) {
	if (file_body->body==NULL) {
		log_stderr_print(51);
		return 1;
	}
	free(file_body->body);
	file_body->body = NULL;
	file_body->size_max = -1;
	file_body->size = -1;
	return 0;
}

int file_body_replace(FILE_BODY *file_body_dest, FILE_BODY *file_body_source) {
	if(file_body_free(file_body_dest)) return 1;
	file_body_dest->body =     file_body_source->body;
	file_body_dest->size =     file_body_source->size;
	file_body_dest->size_max = file_body_source->size_max;
	return 0;
}

int file_read(char *path, FILE_BODY *file_body) {
    FILE * file = fopen(path,"rb");
	if(file==NULL) {
		log_stderr_print(8, path);
		return 1;
	}
	if (file_body_init(file_body)) {
		fclose(file);
		return 1;
	}
	while(!feof(file)) {
		char buffer[10*1024];
		int size = fread(buffer, 1, sizeof(buffer), file);
		if(ferror(file)) {
			log_stderr_print(10, path);
			file_body_free(file_body);
			fclose(file);
			return 1;
		}
		if (file_body_add_substr(file_body, buffer, 0, size-1)) {
			fclose(file);
			return 1;
		}
	}
    if (fclose(file)) {
		log_stderr_print(14, path);
		file_body_free(file_body);
		return 1;
	}
    return 0;
}

// result:
//    0 - equals
//   -1 - new
//   -2 - different
int file_compare(char *path, FILE_BODY *file_new) {
    struct stat file_stat;
    stat(path, &file_stat);
    if (file_stat.st_dev==0) return -1;
	FILE_BODY file_old;
	if (file_read(path, &file_old)) return 1;
	int result = 0;
	if (file_old.size!=file_new->size)
		result = -2;
	else {
		for(int i=0; i<file_old.size; i++)
			if (file_old.body[i]!=file_new->body[i]) { result = -2; break; }
	}
	file_body_free(&file_old);
    return result;
}


int file_write(char *path, FILE_BODY *file_body) {
    FILE * file = fopen(path,"wb");
	if(file==NULL) {
		log_stderr_print(8, path);
	    return 1;
	}
	int size = fwrite(file_body->body, 1, file_body->size, file);
	if(ferror(file)) {
		log_stderr_print(12, path);
		fclose(file);
		return 1;
	}
	if(size!=file_body->size) {
		log_stderr_print(13, path);
		fclose(file);
		return 1;
	}
    if (fclose(file)) {
		log_stderr_print(14, path);
		return 1;
	}
    return 0;
}

int file_body_add_substr(FILE_BODY *file_body, char *source, int pos_begin,	int pos_end) {
	if (file_body->body == NULL) {
		log_stderr_print(51);
		return 1;
	}
	if (file_body->size + (pos_end - pos_begin) + 1 >= file_body->size_max) {
		int size_max_new = file_body->size + (pos_end - pos_begin) + FILE_BODY_SIZE_STEP;
		char *body_new = malloc(size_max_new);
		if (body_new==NULL) {
			log_stderr_print(9, size_max_new);
			file_body_free(file_body);
			return 1;
		}
		for (int i=0; i <= file_body->size; i++)
			body_new[i] = file_body->body[i];
		free(file_body->body);
		file_body->body = body_new;
		file_body->size_max = size_max_new;
	}
	for (int i = pos_begin; i <= pos_end; i++)
		file_body->body[file_body->size++] = source[i];
	file_body->body[file_body->size] = 0;
	return 0;
}


int file_body_add_str(FILE_BODY *file_body, ...) {

	va_list args;
    va_start(args, &file_body);

    while(1)  {
    	char *s = va_arg(args, char *);
    	if (s==NULL) break;
    	if (file_body_add_substr(file_body, s, 0, strlen(s)-1)) return 1;
    }

    va_end(args);
    return 0;

}

int file_is_dir(char *path, int *is_dir) {
	struct stat path_stat;
	if (stat(path, &path_stat)) {
		if (errno==2) {
			*is_dir = -1;
			return 0;
		}
		log_stderr_print(50, path, errno);
		return 1;
	}
	*is_dir =  S_ISDIR(path_stat.st_mode);
	return 0;
}


int file_extension(char *extension, int extension_size, char *filepath) {
	int i = strlen(filepath);
	while (i>=0)
		if (filepath[--i]=='.') break;
	if (i<0) {
		extension[0]=0;
		return 0;
	}
	return str_substr(extension, extension_size, filepath, i+1, strlen(filepath)-1);
}

int file_filename(char *filename, int filename_size, char *filepath) {
	int i = strlen(filepath);
	while (i>=0)
		if (filepath[--i]==FILE_SEPARATOR[0]) break;
	if (i<0)
		return str_copy(filename, filename, filepath);
	return str_substr(filename, filename_size, filepath, i+1, strlen(filepath)-1);
}


int file_make_dirs(char *path, int is_file) {
	STR_LIST dir_list;
	str_list_split(&dir_list, path, FILE_SEPARATOR[0]);
	log_stdout_println("*** file_make_dirs %d", dir_list.size);
	if (is_file) dir_list.size--;
	char dir[STR_SIZE_MAX] = "";
	for(int i=0; i<dir_list.size; i++) {
		if(str_add(dir, sizeof(dir), dir_list.values[i], NULL)) return 1;
		if (str_add(dir, sizeof(dir), FILE_SEPARATOR, NULL)) return 1;
		int is_dir;
		if (file_is_dir(dir, &is_dir)) return 1;
		if (is_dir==1) continue;
		#ifdef _WIN32
			int res = mkdir(dir);
		#else
			int res = mkdir(dir, 0755);
		#endif
		if (res!=0) {
			log_stderr_print(49, dir, errno);
			return 1;
		}
	}
	return 0;
}
