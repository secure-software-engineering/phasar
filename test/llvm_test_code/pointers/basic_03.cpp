#include <functional>

int main() {
  std::function<int()> Func = []() { return 1; };
  auto *FuncPtr = &Func;
  *FuncPtr = []() { return 2; };
}
