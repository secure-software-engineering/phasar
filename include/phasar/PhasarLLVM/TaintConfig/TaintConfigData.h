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

#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include "llvm/ADT/Twine.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include <unordered_set>

namespace psr {
class TaintConfigData;

template <> struct TaintConfigTraits<TaintConfigData> {
  using n_t = const llvm::Instruction *;
  using v_t = const llvm::Value *;
  using f_t = const llvm::Function *;
};

class TaintConfigData : public TaintConfigBase<TaintConfigData> {
  friend TaintConfigBase;

public:
  TaintConfigData(const llvm::Twine &Path);

  void loadDataFromFile();
  void addDataToFile();

  inline void addSourceValue(v_t Value) { SourceValues.insert(Value); }
  inline void addSinkValue(v_t Value) { SinkValues.insert(Value); }
  inline void addSanitizerValue(v_t Value) { SanitizerValues.insert(Value); }

  inline std::unordered_set<v_t> getSourceValues() const {
    return SourceValues;
  }
  inline std::unordered_set<v_t> getSinkValues() const { return SinkValues; }
  inline std::unordered_set<v_t> getSanitizerValues() const {
    return SanitizerValues;
  }
  inline std::unordered_set<f_t> getFunctions() const { return Functions; }
  inline bool hasFunctions() const { return !Functions.empty(); }

private:
  llvm::Twine Path;
  std::unordered_set<f_t> Functions;
  std::unordered_set<v_t> SourceValues;
  std::unordered_set<v_t> SinkValues;
  std::unordered_set<v_t> SanitizerValues;

  void printImpl(llvm::raw_ostream &OS) const;
};

extern template class TaintConfigBase<TaintConfigData>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H