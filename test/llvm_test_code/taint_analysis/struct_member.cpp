#include <cstdio>

struct Point {
  int X, Y;
};

int main(int argc, char *argv[]) {
  Point p = {42, argc};
  printf("Answer to everything: %d\n", p.X);
  printf("Super secret value: %d\n", p.Y);
  return 0;
}