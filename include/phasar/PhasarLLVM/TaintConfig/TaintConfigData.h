/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H
#define PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H

#include <string>
#include <vector>

namespace psr {
enum class TaintCategory;

struct FunctionData {
  FunctionData() noexcept = default;

  std::string Name;
  TaintCategory ReturnCat{};
  std::vector<uint32_t> SourceValues;
  std::vector<uint32_t> SinkValues;
  std::vector<uint32_t> SanitizerValues;
  bool HasAllSinkParam = false;
};

struct VariableData {
  VariableData() noexcept = default;

  size_t Line{};
  std::string Name;
  std::string Scope;
  TaintCategory Cat{};
};

struct TaintConfigData {
  std::vector<FunctionData> Functions;
  std::vector<VariableData> Variables;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H
