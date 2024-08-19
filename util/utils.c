void utils_initialize(char *log_file) {
	_thread_initialize();
	_log_initialize(log_file);
	_pg_initialize();
}
