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
#include <unordered_set>

namespace psr {
class TaintConfigData;
class LLVMProjectIRDB;

class TaintConfigData {
public:
  TaintConfigData() = default;
  explicit TaintConfigData(const std::string &Filepath);

  const std::unordered_set<std::string> &getAllFunctionRets() const;
  const std::unordered_set<std::string> &getAllFunctionParamsSources() const;
  const std::unordered_set<std::string> &getAllFunctionParamsSinks() const;
  const std::unordered_set<std::string> &getAllFunctionParamsSanitizers() const;

  const std::unordered_set<std::string> &getAllVariableScopes() const;
  const std::unordered_set<std::string> &getAllVariableLines() const;
  const std::unordered_set<std::string> &getAllVariableCats() const;
  const std::unordered_set<std::string> &getAllVariableNames() const;

  const std::unordered_set<std::string> &getAllFunctions() const;
  const std::unordered_set<std::string> &getAllVariables() const;

private:
  std::unordered_set<std::string> Functions;
  std::unordered_set<std::string> Variables;

  std::unordered_set<std::string> FunctionRets;
  std::unordered_set<std::string> FunctionParamsSources;
  std::unordered_set<std::string> FunctionParamsSinks;
  std::unordered_set<std::string> FunctionParamsSanitizers;

  std::unordered_set<std::string> VariableScopes;
  std::unordered_set<std::string> VariableLines;
  std::unordered_set<std::string> VariableCats;
  std::unordered_set<std::string> VariableNames;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H
