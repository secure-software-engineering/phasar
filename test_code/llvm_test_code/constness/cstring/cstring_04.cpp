/* immutable: bar */
#include <cstring>
int main() {
  char foo[10] = "Hello";
  char bar[10] = "Bye";
  strcpy(foo,bar);
  return 0;
}