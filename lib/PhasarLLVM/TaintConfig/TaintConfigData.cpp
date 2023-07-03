#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"

#include "phasar/Utils/NlohmannLogging.h"

#include <fstream>

namespace psr {

TaintConfigData::TaintConfigData(const llvm::Twine &Path) : Path(Path) {}

void TaintConfigData::loadDataFromFile() {
  // retrieve data from file
  nlohmann::json Config = parseTaintConfig(Path);

  // load data from nlohmann::json
}

void TaintConfigData::addDataToFile() {
  nlohmann::json Config;

  // TODO (max): add data to nlohmann::json Config
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

} // namespace psr
