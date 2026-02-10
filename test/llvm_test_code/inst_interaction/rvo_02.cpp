#include <string>

int g = 0;
void functionWithoutInput() { g = 42; }
std::string createString() { return "My String"; }

int main() {
  std::string str;
  functionWithoutInput();
  str = "1234";
  str = createString();
  return str.size();
}
