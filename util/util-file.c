#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include "util-file.h"

int file_read(char *path, FILE_BODY *file_body) {
    FILE * file = fopen(path,"rb");
	if(file==NULL) {
		stderr_printf(8, path);
		return 1;
	}
	char *body=malloc(FILE_SIZE_MAX);
	if (body==NULL) {
		stderr_printf(9, FILE_SIZE_MAX);
	    fclose(file);
		return 1;
	}
	int size = fread(body, 1, FILE_SIZE_MAX-1, file);
	if(ferror(file)) {
		stderr_printf(10, path);
		free(body);
		fclose(file);
		return 1;
	}
	if(!feof(file)) {
		stderr_printf(11, path);
		free(body);
		fclose(file);
		return 1;
	}
    if (fclose(file)) {
		stderr_printf(14, path);
		return 1;
	}
    body[size]=0;
    file_body->body=body;
	file_body->size=size;
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
	free(file_old.body);
    return result;
}


int file_write(char *path, FILE_BODY *file_body) {
    FILE * file = fopen(path,"wb");
	if(file==NULL) {
		stderr_printf(8, path);
	    return 1;
	}
	int size = fwrite(file_body->body, 1, file_body->size, file);
	if(ferror(file)) {
		stderr_printf(12, path);
		fclose(file);
		return 1;
	}
	if(size!=file_body->size) {
		stderr_printf(13, path);
		fclose(file);
		return 1;
	}
    if (fclose(file)) {
		stderr_printf(14, path);
		return 1;
	}
    return 0;
}

int file_add(FILE_BODY *file_body, char *source, int pos_begin, int pos_end) {
	if (file_body->size+(pos_end-pos_begin)>=FILE_SIZE_MAX) {
		stderr_printf(15, file_body->size+(pos_end-pos_begin));
		return 1;
	}
	for (int i=pos_begin; i<=pos_end; i++)
		file_body->body[file_body->size++] = source[i];
	return 0;
}

int file_add_eol(FILE_BODY *file_body) {
	return file_add(file_body, EOL, 0, EOL_LEN-1);
}

int file_body_empty(FILE_BODY *filebody) {
	filebody->body = malloc(FILE_SIZE_MAX);
	if (filebody->body==NULL) {
		stderr_printf(9, FILE_SIZE_MAX);
		return 1;
	}
	filebody->size = 0;
	return 0;
}

int file_is_dir(char *filepath) {
	struct stat file_stat;
	stat(filepath, &file_stat);
	return S_ISDIR(file_stat.st_mode);
}


int file_extention(char *extention, int extention_size, char *filepath) {
	int i = strlen(filepath);
	while (i>=0)
		if (filepath[--i]=='.') break;
	if (i<0) {
		extention[0]=0;
		return 0;
	}
	return str_substr(extention, extention_size, filepath, i+1, strlen(filepath)-i-1);
}

int file_filename(char *filename, int filename_size, char *filepath) {
	int i = strlen(filepath);
	while (i>=0)
		if (filepath[--i]==FILE_SEPARATOR[0]) break;
	if (i<0)
		return str_copy(filename, filename, filepath);
	return str_substr(filename, filename_size, filepath, i+1, strlen(filepath)-i-1);
}
