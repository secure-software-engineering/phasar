#include "phasar/Utils/AnalysisProperties.h"

#include "phasar/Utils/EnumFlags.h"

#include "llvm/Support/raw_ostream.h"

std::string psr::to_string(AnalysisProperties Prop) {
  if (Prop == AnalysisProperties::None) {
    return "None";
  }

  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);
  if (hasFlag(Prop, AnalysisProperties::FlowSensitive)) {
    ROS << "FlowSensitive";
  }
  if (hasFlag(Prop, AnalysisProperties::ContextSensitive)) {
    ROS << (Ret.empty() ? "ContextSensitive" : " | ContextSensitive");
  }
  if (hasFlag(Prop, AnalysisProperties::FieldSensitive)) {
    ROS << (Ret.empty() ? "FieldSensitive" : " | FieldSensitive");
  }

  return Ret;
}
