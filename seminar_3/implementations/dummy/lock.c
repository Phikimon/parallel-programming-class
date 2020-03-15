void* lock_alloc(void) {
	return (void*)(-1);
}

int lock_acquire(void* arg) {
	return 1;
}

int lock_release(void* arg) {
	return 1;
}

int lock_free(void* arg) {
	return 1;
}
