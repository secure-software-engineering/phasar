extern int pthread_create(void **thread, const void *attr);

int main() { return pthread_create(0, 0); }
