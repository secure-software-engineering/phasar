#include <string>

int main() {
  std::string i;
  std::string *p = &i;
  *p = "Six";
}
