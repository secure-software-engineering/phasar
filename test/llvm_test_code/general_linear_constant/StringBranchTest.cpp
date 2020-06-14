#include <string>

int rand();
void puts(std::string);

int main() {
  std::string str1("Hello, World");
  std::string str2("Hello Hello");

  if (rand()) {
    str1 = str2;
  }

  puts(str1);
  return 0;
}
