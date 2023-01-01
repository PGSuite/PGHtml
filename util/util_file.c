#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>

#include "utils.h"

int file_read(char *path, stream *stream) {
    FILE *file = fopen(path,"rb");
	if(file==NULL) {
		log_stderr_print(8, path);
		return 1;
	}
	if (stream_init(stream)) {
		fclose(file);
		return 1;
	}
	while(!feof(file)) {
		char buffer[10*1024];
		int size = fread(buffer, 1, sizeof(buffer), file);
		if(ferror(file)) {
			log_stderr_print(10, path);
			stream_free(stream);
			fclose(file);
			return 1;
		}
		if (stream_add_substr(stream, buffer, 0, size-1)) {
			fclose(file);
			return 1;
		}
	}
    if (fclose(file)) {
		log_stderr_print(14, path);
		stream_free(stream);
		return 1;
	}
    return 0;
}

// result:
//   >0 - error
//    0 - equals
//   -1 - new
//   -2 - different
int file_compare(char *path, stream *file_new) {
    struct stat file_stat;
    stat(path, &file_stat);
    if (file_stat.st_dev==0) return -1;
	stream file_old;
	if (file_read(path, &file_old)) return 1;
	int result = 0;
	if (file_old.len!=file_new->len)
		result = -2;
	else {
		for(int i=0; i<file_old.len; i++)
			if (file_old.data[i]!=file_new->data[i]) { result = -2; break; }
	}
	stream_free(&file_old);
    return result;
}


int file_write(char *path, stream *stream) {
    FILE * file = fopen(path,"wb");
	if(file==NULL) {
		log_stderr_print(8, path);
	    return 1;
	}
	int size = fwrite(stream->data, 1, stream->len, file);
	if(ferror(file)) {
		log_stderr_print(12, path);
		fclose(file);
		return 1;
	}
	if(size!=stream->len) {
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
	*is_dir = S_ISDIR(path_stat.st_mode);
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
	str_list dir_list;
	str_list_split(&dir_list, path, 0, -1, FILE_SEPARATOR[0]);
	if (is_file) dir_list.len--;
	char dir[STR_SIZE] = "";
	for(int i=0; i<dir_list.len; i++) {
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
