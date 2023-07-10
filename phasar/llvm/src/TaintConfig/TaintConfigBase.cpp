#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

#include "nlohmann/json-schema.hpp"
#include "nlohmann/json.hpp"

#include <optional>

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

nlohmann::json psr::parseTaintConfig(const llvm::Twine &Path) {
  auto Ret = parseTaintConfigOrNull(Path);
  if (!Ret) {
    return {};
  }
  return std::move(*Ret);
}

std::optional<nlohmann::json>
psr::parseTaintConfigOrNull(const llvm::Twine &Path) {
  std::optional<nlohmann::json> TaintConfig = readJsonFile(Path);
  nlohmann::json_schema::json_validator Validator;
  try {
    static const nlohmann::json TaintConfigSchema =
#include "config/TaintConfigSchema.json"
        ;

    Validator.set_root_schema(TaintConfigSchema); // insert root-schema
  } catch (const std::exception &E) {
    PHASAR_LOG_LEVEL(ERROR,
                     "Validation of schema failed, here is why: " << E.what());
    return std::nullopt;
  }

  // a custom error handler
  class CustomJsonErrorHandler
      : public nlohmann::json_schema::basic_error_handler {
    void error(const nlohmann::json::json_pointer &Pointer,
               const nlohmann::json &Instance,
               const std::string &Message) override {
      nlohmann::json_schema::basic_error_handler::error(Pointer, Instance,
                                                        Message);
      PHASAR_LOG_LEVEL(ERROR, Pointer.to_string()
                                  << "' - '" << Instance << "': " << Message);
    }
  };
  CustomJsonErrorHandler Err;
  Validator.validate(*TaintConfig, Err);
  if (Err) {
    TaintConfig.reset();
  }
  return TaintConfig;
}
