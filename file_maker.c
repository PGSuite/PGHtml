#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include "globals.h"
#include "util/utils.h"

void* file_maker_thread(void *args) {

	thread_begin(args);

	PGconn *pg_conn;

	for(;1;sleep(http_sync_interval)) {
		if (pg_connect(&pg_conn, db_service_uri))
			continue;
		file_maker_sync_dir(pg_conn);
		pg_disconnect(&pg_conn);
	}

	thread_end(args);
	return 0;

}

int file_maker_sync_dir(PGconn *pg_conn) {
	log_info("syncing %s directory", http_directory);
	int res = file_maker_sync_subdir(pg_conn, "");
	log_info("directory synced %s", res ? "with error(s)" : "successfully");
	return res;
}

int file_maker_sync_subdir(PGconn *pg_conn, char *directory) {
	int result = 0;
	char dir_path[STR_SIZE] = "";
	if(str_add(dir_path, sizeof(dir_path), http_directory, FILE_SEPARATOR, directory, NULL)) return 1;
    DIR *dir = opendir(dir_path);
    struct dirent *dir_ent;
    while ((dir_ent = readdir(dir)) != NULL) {
    	if (dir_ent->d_name[0]=='.') continue;
		char path_ent[STR_SIZE] = "";
		if (str_add(path_ent, sizeof(path_ent), http_directory, FILE_SEPARATOR, directory, dir_ent->d_name, NULL))  { result = 1; break; }
		int is_dir;
		if (file_is_dir(path_ent, &is_dir)) return 1;
		if (is_dir) {
			char directory_ent[STR_SIZE] = "";
			if (str_add(directory_ent, sizeof(path_ent), directory, dir_ent->d_name, FILE_SEPARATOR, NULL)) { result = 1; break; }
			if (file_maker_sync_subdir(pg_conn, directory_ent))
				result = 1;
			continue;
		}
		if (template_maker_process(pg_conn, directory, dir_ent->d_name))
			result = 1;
    }
    closedir(dir);
    return result;
}
