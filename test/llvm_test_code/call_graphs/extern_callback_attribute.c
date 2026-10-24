extern void callback_broker(void (*callback)(int), int value)
    __attribute__((callback(1, 2)));

void sink(int value) { (void)value; }

int main() {
  callback_broker(sink, 42);
  return 0;
}
