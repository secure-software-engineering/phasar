/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_CONSTANTHELPER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_CONSTANTHELPER_H_

namespace llvm {
class Value;
} // namespace llvm

namespace psr {
bool isConstant(const llvm::Value *Val);
} // namespace psr

#endif
