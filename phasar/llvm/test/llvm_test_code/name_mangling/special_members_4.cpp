#include <utility>

namespace C0C1C2C1 {

struct D1 {
  D1() {}
  ~D1() {}
  D1(const D1 &d) {}
  D1(D1 &&d) {}
};
} // namespace C0C1C2C1

void C1C2C3D0D1D2() {}

int main(int argc, char const *argv[]) {
  C1C2C3D0D1D2();
  C0C1C2C1::D1 d;
  C0C1C2C1::D1 d1(d);
  C0C1C2C1::D1 d2(std::move(d));
  return 0;
}
