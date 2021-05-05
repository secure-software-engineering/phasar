#include <string>

std::string createString() { return "My String"; }

int main() {
  std::string str;
  str = createString();
  return str.size();
}
