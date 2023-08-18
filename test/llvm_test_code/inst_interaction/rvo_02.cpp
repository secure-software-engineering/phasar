#include <cstddef>
#include <cstring>

template <typename T> struct RemoveReference {
  using type = T;
};
template <typename T> struct RemoveReference<T &> {
  using type = T;
};

template <typename T>
__attribute__((always_inline)) constexpr decltype(auto) move(T &Obj) noexcept {
  return static_cast<typename RemoveReference<T>::type &&>(Obj);
}

template <typename T> T &&declval() noexcept;

template <typename T>
constexpr void defaultSwap(T &L, T &R) noexcept(noexcept(T(declval<T>()))) {
  if (&L == &R) [[unlikely]]
    return;

  T Tmp = move(L);
  L = move(R);
  R = move(Tmp);
}

class String {
public:
  constexpr String() noexcept = default;

  String(const char *CStr) : String(CStr, CStr ? strlen(CStr) : 0) {}
  ~String() { delete[] Data; }
  explicit String(const String &Other) : String(Other.Data, Other.Length) {}
  constexpr String(String &&Other) noexcept
      : Data(Other.Data), Length(Other.Length) {
    Other.Data = nullptr;
    Other.Length = 0;
  }
  constexpr void swap(String &Other) noexcept {
    defaultSwap(Data, Other.Data);
    defaultSwap(Length, Other.Length);
  }

  String &operator=(const String &Other) = delete;
  String &operator=(String &&Other) noexcept {
    String Tmp = move(Other);
    swap(Tmp);
    return *this;
  }

  [[nodiscard]] constexpr size_t size() const noexcept { return Length; }
  [[nodiscard]] constexpr bool empty() const noexcept { return Length == 0; }

private:
  explicit String(const char *Buf, size_t Len) : Length(Len) {
    auto *DataMut = new char[Length + 1];
    memcpy(DataMut, Buf, Length);
    DataMut[Length] = '\0';
    Data = DataMut;
  }

  const char *Data = nullptr;
  size_t Length = 0;
};

String createString() { return "My String"; }

int main() {
  String str;
  str = createString();
  return str.size();
}
