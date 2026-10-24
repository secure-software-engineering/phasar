extern void callback_broker(void (*callback)(int), int value)
    __attribute__((callback(1, 2)));

void sink(int value, int extra) {
  (void)value;
  (void)extra;
}

int main() {
  callback_broker((void (*)(int))sink, 42);
  return 0;
}
