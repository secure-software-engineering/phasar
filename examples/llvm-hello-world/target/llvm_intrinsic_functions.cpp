#include <cstring>

int main() {
  char buffer[10];
  memset(buffer, 0, sizeof(buffer));
  int i = buffer[3] + 42;
  return 0;
}
