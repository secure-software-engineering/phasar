#include <stdio.h>

extern char *source(void); // dummy source
extern void sink(char c);  // dummy sink

typedef struct {
  char data[64];
  int x;
} BigStruct;

BigStruct get_data(char *input) {
  BigStruct s;
  s.data[0] = input[0];
  s.x = 42;
  return s;
}

int main() {
  char *tainted = source();
  BigStruct bs = get_data(tainted); // sret call
  sink(bs.data[0]);                 // Should be flagged as leak!
  return 0;
}
