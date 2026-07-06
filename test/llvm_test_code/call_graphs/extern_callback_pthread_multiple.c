extern int pthread_create(void **thread, const void *attr,
                          void *(*start_routine)(void *), void *arg);

void *first_worker(void *arg) { return arg; }

void *second_worker(void *arg) { return arg; }

int main() {
  void *first_thread = 0;
  void *second_thread = 0;
  void *payload = 0;
  pthread_create(&first_thread, 0, first_worker, payload);
  pthread_create(&second_thread, 0, second_worker, payload);
  return 0;
}
