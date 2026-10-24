extern void callback_broker(void (*callback)(int), int value)
    __attribute__((callback(1, 2)));

void sink(int value) { (void)value; }

int main() {
  void (*cb)(int) = sink;
  callback_broker(cb, 42);
  return 0;
}
