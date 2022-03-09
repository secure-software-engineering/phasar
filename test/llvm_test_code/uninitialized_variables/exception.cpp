#include <stdexcept>

int myfunction(int a) { throw std::runtime_error("error occured"); }

int main() {
  try {
    int val = 13;
    int i = myfunction(val);
  } catch (std::runtime_error &e) {
    e.what();
  }

  return 0;
}
