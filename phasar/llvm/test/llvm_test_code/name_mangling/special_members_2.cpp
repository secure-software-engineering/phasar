#include <iostream>
using namespace std;

int main() {
  std::string c("test");
  std::string d(c);
  std::cout << c << d << "\n";
  std::string e, f;
  e = c;
  std::cout << e << "\n";
  std::string g(move(e));
  std::cout << g << "\n";
  f = move(d);
  std::cout << f << "\n";
  return 0;
}
