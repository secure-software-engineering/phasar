extern void callback_broker(void (*callback)(int), int value, int extra)
    __attribute__((callback(1, 3)));

void sink(int value) { (void)value; }

int main() {
  ((void (*)(void (*)(int), int))callback_broker)(sink, 42);
  return 0;
}
