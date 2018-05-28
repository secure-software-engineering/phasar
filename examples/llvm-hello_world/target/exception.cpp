#include <stdexcept>

void may_throw(int i) {
  if (i == 1) {
    throw std::runtime_error("error");
  }
}

int main(int argc, char **argv) {
  bool has_thrown = false;
  try {
    may_throw(argc);
  } catch (std::runtime_error &e) {
    has_thrown = true;
  }
  return has_thrown;
}
