#include <string>
int describe() { return static_cast<int>(std::string{"phasar"}.size()); }
int main() { return describe(); }
