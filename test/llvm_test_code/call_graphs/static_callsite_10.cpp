// Handle Lambda function call function
#include <functional>

int main() {

  std::function<void()> foo = []() {};
  foo();
}
