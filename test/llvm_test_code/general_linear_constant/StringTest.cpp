#include <string>

void puts(const std::string);

int main() {
  const std::string str1 = "Hello, World";
  const std::string str2 = str1;
  puts(str2);
  return 0;
}
