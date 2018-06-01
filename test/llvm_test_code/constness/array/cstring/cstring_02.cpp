/* a | %1 (ID: 2) | mem2reg */
#include <cstring>
int main() {
  char a[10] = "Hello";
  char b[10] = "Bye";
  strcpy(a,b);
  return 0;
}