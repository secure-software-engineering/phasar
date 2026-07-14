extern int pthread_create(void **thread, const void *attr,
                          void *(*start_routine)(void *), void *arg);

void *worker(void *arg) { return arg; }

int main() {
  void *thread = 0;
  void *payload = 0;
  return pthread_create(&thread, 0, worker, payload);
}
