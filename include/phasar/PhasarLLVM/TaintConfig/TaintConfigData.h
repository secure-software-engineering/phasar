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
struct TaintConfigData;
class LLVMProjectIRDB;

struct FunctionData {
  FunctionData() = default;

  std::string Name;
  std::string ReturnType;
  std::vector<int> SourceValues;
  std::vector<int> SinkValues;
  std::vector<int> SanitizerValues;
};

struct VariableData {
  VariableData() = default;

  size_t Line{};
  std::string Name;
  std::string Scope;
  std::string Cat;
};

struct TaintConfigData {
  TaintConfigData() = default;
  explicit TaintConfigData(const std::string &Filepath);

  std::vector<FunctionData> Functions;
  std::vector<VariableData> Variables;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H
