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

#include <iosfwd>
#include <vector>

#include <nlohmann/json.hpp>

#include <phasar/PhasarLLVM/TypeHierarchy/VFTable.h>

namespace llvm {
class Function;
} // namespace llvm

namespace psr {

/**
 * 	@brief Represents a virtual method table.
 *
 * 	Note that the position of a function identifier in the
 * 	virtual method table matters.
 */
class LLVMVFTable : public VFTable<const llvm::Function *> {
private:
  friend class LLVMTypeHierarchy;
  std::vector<const llvm::Function *> VFT;
  LLVMVFTable(std::vector<const llvm::Function *> Fs);

public:
  LLVMVFTable() = default;
  ~LLVMVFTable() override = default;

  /**
   * 	@brief Returns a function identifier by it's index in the VTable.
   * 	@param i Index of the entry.
   * 	@return Function identifier.
   */
  const llvm::Function *getFunction(unsigned Idx) const override;

  std::vector<const llvm::Function *> getAllFunctions() const override;

  /**
   * 	@brief Returns position index of the given function identifier
   * 	       in the VTable.
   * 	@param fname Function identifier.
   * 	@return Index of the functions entry.
   */
  int getIndex(const llvm::Function *F) const override;

  bool empty() const override;

  size_t size() const override;

  void print(std::ostream &OS) const override;

  nlohmann::json getAsJson() const override;

  std::vector<const llvm::Function *>::iterator begin();

  std::vector<const llvm::Function *>::const_iterator begin() const;

  std::vector<const llvm::Function *>::iterator end();

  std::vector<const llvm::Function *>::const_iterator end() const;
};

} // namespace psr

#endif
