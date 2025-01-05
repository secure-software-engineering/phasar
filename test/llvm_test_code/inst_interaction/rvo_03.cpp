
#include <cstddef>
#include <cstring>
#include <utility>

class String {
public:
  String() noexcept = default;

  String(const char *Data) noexcept : Length(strlen(Data)) {
    auto *Dat = new char[Length];
    this->Data = Dat;
    memcpy(Dat, Data, Length);
  }
  ~String() { delete[] Data; }

  String(String &&Other) noexcept : Data(Other.Data), Length(Other.Length) {
    Other.Data = nullptr;
    Other.Length = 0;
  }

  void swap(String &Other) noexcept {
    const auto *Dat = Data;
    Data = Other.Data;
    Other.Data = Dat;

    auto Len = Length;
    Length = Other.Length;
    Other.Length = Len;
  }

  String &operator=(String &&Other) noexcept {
    String(std::move(Other)).swap(*this);
    return *this;
  }

  [[nodiscard]] size_t size() const noexcept { return Length; }

private:
  const char *Data{};
  size_t Length{};
};

int g = 0;
void functionWithoutInput() { g = 42; }
String createString() noexcept { return "My String"; }

int main() {
  String Str;
  functionWithoutInput();
  Str = "1234";
  Str = createString();
  return Str.size();
}
