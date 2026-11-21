
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

} // namespace

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const nlohmann::json &J) {
  // do the actual serialization
  LLVMOutputStreamAdapter Adapter(OS);
  nlohmann::detail::serializer<nlohmann::json> S{
      Adapter,
      /*ichar=*/' ',
      /*pretty_print_=*/true,
      /*ensure_ascii_=*/false,
      /*indent_step_=*/4,
  };
  S.dump(J);
  return OS;
}

} // namespace psr
