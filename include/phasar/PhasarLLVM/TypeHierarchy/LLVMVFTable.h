/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMVFTABLE_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMVFTABLE_H_

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTableData.h"
#include "phasar/TypeHierarchy/VFTable.h"

#include "nlohmann/json.hpp"

#include <vector>

namespace llvm {
class Function;
class ConstantStruct;
} // namespace llvm

namespace psr {
class DIBasedTypeHierarchy;

/**
 * 	@brief Represents a virtual method table.
 *
 * 	Note that the position of a function identifier in the
 * 	virtual method table matters.
 */
class LLVMVFTable : public VFTable<const llvm::Function *> {

private:
  friend class DIBasedTypeHierarchy;
  std::vector<const llvm::Function *> VFT;

public:
  // NOLINTNEXTLINE
  static constexpr char NullFunName[] = "__null__";

  LLVMVFTable() = default;
  LLVMVFTable(std::vector<const llvm::Function *> Fs) : VFT(std::move(Fs)) {}
  ~LLVMVFTable() override = default;

  /**
   * 	@brief Returns a function identifier by it's index in the VTable.
   * 	@param i Index of the entry.
   * 	@return Function identifier.
   */
  [[nodiscard]] const llvm::Function *getFunction(unsigned Idx) const override;

  [[nodiscard]] std::vector<const llvm::Function *>
  getAllFunctions() const override {
    return VFT;
  }

  /**
   * 	@brief Returns position index of the given function identifier
   * 	       in the VTable.
   * 	@param fname Function identifier.
   * 	@return Index of the functions entry.
   */
  int getIndex(const llvm::Function *F) const override;

  [[nodiscard]] bool empty() const override { return VFT.empty(); };

  [[nodiscard]] size_t size() const override { return VFT.size(); };

  void print(llvm::raw_ostream &OS) const override;

  [[nodiscard]] [[deprecated(
      "Please use printAsJson() instead")]] nlohmann::json
  getAsJson() const override;

  [[nodiscard]] LLVMVFTableData getVFTableData() const;

  void printAsJson(llvm::raw_ostream &OS) const override;

  [[nodiscard]] std::vector<const llvm::Function *>::iterator begin() {
    return VFT.begin();
  }

  [[nodiscard]] std::vector<const llvm::Function *>::const_iterator
  begin() const {
    return VFT.begin();
  };

  [[nodiscard]] std::vector<const llvm::Function *>::iterator end() {
    return VFT.end();
  };

  [[nodiscard]] std::vector<const llvm::Function *>::const_iterator
  end() const {
    return VFT.end();
  };

  [[nodiscard]] static std::vector<const llvm::Function *>
  getVFVectorFromIRVTable(const llvm::ConstantStruct &);
};

} // namespace psr

#endif
