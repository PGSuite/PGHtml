#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>

#include "utils.h"

int file_read(char *path, stream *file_body) {
    FILE *file = fopen(path,"rb");
	if(file==NULL)
		return log_error(8, path);
	if (stream_init(file_body)) {
		fclose(file);
		return 1;
	}
	while(!feof(file)) {
		char buffer[10*1024];
		int size = fread(buffer, 1, sizeof(buffer), file);
		if(ferror(file)) {
			stream_free(file_body);
			fclose(file);
			return log_error(10, path);
		}
		if (size>0 && stream_add_substr(file_body, buffer, 0, size-1)) {
			fclose(file);
			return 1;
		}
	}
    if (fclose(file)) {
    	stream_free(file_body);
    	return log_error(14, path);
	}
    return 0;
}

int file_write(char *path, stream *file_body) {
    FILE * file = fopen(path,"wb");
	if(file==NULL)
		return log_error(8, path);
	int offset = 0;
	while(offset<file_body->len) {
		int len = fwrite((file_body->data)+offset, 1, file_body->len-offset, file);
		if(ferror(file)) {
			fclose(file);
			return log_error(12, path);
		}
		if(len==0) {
			fclose(file);
			return log_error(13, path);
		}
		offset += len;
	}
    if (fclose(file))
    	return log_error(14, path);
    return 0;
}

// result:
//   >0 - error
//   -1 - new
//   -2 - changed
//   -3 - equals
int file_write_if_changed(char *path, stream *file_body) {
    struct stat file_stat;
    int result;
    if (stat(path, &file_stat) || file_stat.st_dev==0)
    	result = -1;
    else {
    	stream file_body_old;
    	if (file_read(path, &file_body_old)) return 1;
    	if (file_body_old.len!=file_body->len)
    		result = -2;
    	else {
    		result = -3;
    		for(int i=0; i<file_body_old.len; i++)
    			if (file_body_old.data[i]!=file_body->data[i]) { result = -2; break; }
    	}
    	stream_free(&file_body_old);
    }
    if (result!=-3 && file_write(path, file_body)) return 1;
    return result;
}

int file_is_dir(char *path, int *is_dir) {
	struct stat path_stat;
	if (stat(path, &path_stat)) {
		if (errno==2) {
			*is_dir = -1;
			return 0;
		}
		return log_error(50, path, errno);
	}
	*is_dir = S_ISDIR(path_stat.st_mode);
	return 0;
}

int file_remove(char *filepath, int remove_empty_dir) {
	if (remove(filepath))
		if(errno!=2) return log_error(78, filepath, errno);
	if (remove_empty_dir) {
		char dir[STR_SIZE];
		if (file_dir(dir, sizeof(dir), filepath))
			return 1;
		if (rmdir(dir))
			if(errno!=39 && errno!=41) return log_error(79, dir, errno);
	}
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
		return str_copy(filename, filename_size, filepath);
	return str_substr(filename, filename_size, filepath, i+1, strlen(filepath)-1);
}

int file_dir(char *dir, int dir_size, char *filepath) {
	int i = strlen(filepath);
	while (i>=0)
		if (filepath[--i]==FILE_SEPARATOR[0]) break;
	if (i<=0) {
		dir[0] = 0;
		return 0;
	}
	return str_substr(dir, dir_size, filepath, 0, i-1);
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
		if (res!=0)
			return log_error(49, dir, errno);
	}
	return 0;
}
