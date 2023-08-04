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
class TaintConfigData;
class LLVMProjectIRDB;

class TaintConfigData {
public:
  TaintConfigData() = default;
  explicit TaintConfigData(const std::string &Filepath);

  [[nodiscard]] const std::vector<std::string> &getAllFunctionNames() const;
  [[nodiscard]] const std::vector<std::string> &getAllFunctionRets() const;
  [[nodiscard]] const std::vector<std::string> &
  getAllFunctionParamsSources() const;
  [[nodiscard]] const std::vector<std::string> &
  getAllFunctionParamsSinks() const;
  [[nodiscard]] const std::vector<std::string> &
  getAllFunctionParamsSanitizers() const;

  [[nodiscard]] const std::vector<std::string> &getAllVariableScopes() const;
  [[nodiscard]] const std::vector<std::string> &getAllVariableLines() const;
  [[nodiscard]] const std::vector<std::string> &getAllVariableCats() const;
  [[nodiscard]] const std::vector<std::string> &getAllVariableNames() const;

  [[nodiscard]] const std::vector<std::string> &getAllFunctions() const;
  [[nodiscard]] const std::vector<std::string> &getAllVariables() const;

private:
  std::vector<std::string> Functions;
  std::vector<std::string> Variables;

  std::vector<std::string> FunctionNames;
  std::vector<std::string> FunctionRets;
  std::vector<std::string> FunctionParamSources;
  std::vector<std::string> FunctionParamSinks;
  std::vector<std::string> FunctionParamSanitizers;

  std::vector<std::string> VariableScopes;
  std::vector<std::string> VariableLines;
  std::vector<std::string> VariableCats;
  std::vector<std::string> VariableNames;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H
