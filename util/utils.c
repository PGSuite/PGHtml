void utils_initialize(char *error_prefix) {
	_log_initialize(error_prefix);
	_thread_initialize();
	_pg_initialize();
}
