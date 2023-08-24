#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

#include <optional>

#include <nlohmann/json-schema.hpp>

llvm::StringRef psr::to_string(TaintCategory Cat) noexcept {
  switch (Cat) {
  case TaintCategory::Source:
    return "Source";
  case TaintCategory::Sink:
    return "Sink";
  case TaintCategory::Sanitizer:
    return "Sanitizer";
  case TaintCategory::None:
    return "None";
  }
  llvm_unreachable("Invalid TaintCategory");
}

psr::TaintCategory psr::toTaintCategory(llvm::StringRef Str) noexcept {
  return llvm::StringSwitch<TaintCategory>(Str)
      .CaseLower("source", TaintCategory::Source)
      .CaseLower("sink", TaintCategory::Sink)
      .CaseLower("sanitizer", TaintCategory::Sanitizer)
      .Default(TaintCategory::None);
}

psr::TaintConfigData psr::parseTaintConfig(const llvm::Twine &Path) {
  auto Ret = parseTaintConfigOrNull(Path);
  if (!Ret) {
    return {};
  }
  return std::move(*Ret);
}

std::optional<psr::TaintConfigData>
psr::parseTaintConfigOrNull(const llvm::Twine &Path) {
  return TaintConfigData(Path.str());
}
