
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

namespace psr {

namespace {

// See nlohmann/detail/output/output_adapters.hpp for reference

class LLVMOutputStreamAdapter
    : public nlohmann::detail::output_adapter_protocol<char> {
public:
  explicit LLVMOutputStreamAdapter(llvm::raw_ostream &OS) noexcept : OS(OS) {}

  void write_character(char C) override { OS.write(C); }

  void write_characters(const char *Str, std::size_t Length) override {
    OS.write(Str, Length);
  }

private:
  llvm::raw_ostream &OS;
};

template <typename StringType = std::basic_string<char>>
class LLVMOutputAdapter {
public:
  template <typename AllocatorType = std::allocator<char>>

  LLVMOutputAdapter(llvm::raw_ostream &OS)
      : OA(std::make_shared<LLVMOutputStreamAdapter>(OS)) {}

  operator nlohmann::detail::output_adapter_t<char>() { return OA; }

private:
  nlohmann::detail::output_adapter_t<char> OA = nullptr;
};
} // namespace

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const nlohmann::json &J) {
  // do the actual serialization
  nlohmann::detail::serializer<nlohmann::json> S(LLVMOutputAdapter(OS), ' ');
  S.dump(J, true, false, 4);
  return OS;
}

} // namespace psr
