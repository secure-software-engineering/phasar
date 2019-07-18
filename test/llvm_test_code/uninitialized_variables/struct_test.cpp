#include <cstdio>

struct MyStruct {
  int x, y;
};
int main() {
  MyStruct s;
  printf("(%d, %d)\n", s.x, s.y);
  return 0;
}