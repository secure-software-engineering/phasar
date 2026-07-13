extern void callback_broker(void (*callback)(int), int value)
    __attribute__((callback(1, 2)));

int main() {
  ((void (*)(int, int))callback_broker)(1, 42);
  return 0;
}
