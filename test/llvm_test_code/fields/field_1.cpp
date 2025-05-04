#include <iostream>

struct Data {
  int x;
  int y;
  int *z_ptr;
};

int main() {
  Data d;
  int i = 3;
  d.x = 1;
  d.y = 2;
  d.z_ptr = &i;
  std::cout << d.x << ", " << d.y << ", " << *d.z_ptr << '\n';
}
