#include <cstdio>
#include <memory>
using namespace std;

int main(int argc, char *argv[]) {
  auto i = new int(argc);
  printf("Leak secret value: %d\n", *i);
  delete i;
}