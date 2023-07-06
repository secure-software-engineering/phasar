#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/NlohmannLogging.h"

#include <fstream>

namespace psr {

TaintConfigData::TaintConfigData(const llvm::Twine &Path) : Path(Path) {}

void TaintConfigData::loadDataFromFile() {
  nlohmann::json Config = parseTaintConfig(Path);
}

void TaintConfigData::addDataToFile() {
  nlohmann::json Config;

  for (const auto &Source : SourceValues) {
    Config.push_back({"SourceValues", {{Source->getName().str()}}});
  }

  for (const auto &Sink : SinkValues) {
    Config.push_back({"SinkValues", {{Sink->getName().str()}}});
  }

  for (const auto &Sanitizer : SanitizerValues) {
    Config.push_back({"SanitizerValues", {{Sanitizer->getName().str()}}});
  }

  std::ofstream File(Path.str());
  File << Config;
}

void TaintConfigData::printImpl(llvm::raw_ostream &OS) const {
  OS << "TaintConfiguration in TaintConfigData: ";
  if (SourceValues.empty() && SinkValues.empty() && SanitizerValues.empty() &&
      !getRegisteredSourceCallBack() && !getRegisteredSinkCallBack()) {
    OS << "empty";
    return;
  }
  OS << "\n\tSourceCallBack registered: " << (bool)SourceCallBack << '\n';
  OS << "\tSinkCallBack registered: " << (bool)SinkCallBack << '\n';
  OS << "\tSources (" << SourceValues.size() << "):\n";
  for (const auto *SourceValue : SourceValues) {
    OS << "\t\t" << psr::llvmIRToString(SourceValue) << '\n';
  }
  OS << "\tSinks (" << SinkValues.size() << "):\n";
  for (const auto *SinkValue : SinkValues) {
    OS << "\t\t" << psr::llvmIRToString(SinkValue) << '\n';
  }
  OS << "\tSanitizers (" << SanitizerValues.size() << "):\n";
  for (const auto *SanitizerValue : SanitizerValues) {
    OS << "\t\t" << psr::llvmIRToString(SanitizerValue) << '\n';
  }
}

} // namespace psr
