/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEInstInteractionAnalysis.h"

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"

namespace psr {

void AnalysisController::executeIDEIIA() {
  auto EdgeFactGen =
      [](std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>
             Src) -> std::set<std::string> {
    return std::visit(
        [](const auto *Inst) -> std::set<std::string> {
          const auto *FVarAnnot = Inst->getMetadata("FVar");
          if (!FVarAnnot || FVarAnnot->getNumOperands() == 0) {
            return {};
          }

          const auto *FVar =
              llvm::dyn_cast<llvm::MDNode>(FVarAnnot->getOperand(0).get());
          if (!FVar || FVar->getNumOperands() == 0) {
            return {};
          }

          if (const auto *Feat =
                  llvm::dyn_cast<llvm::MDString>(FVar->getOperand(0).get())) {
            return {Feat->getString().str()};
          }
          return {};
        },
        Src);
  };

  executeIDEAnalysis<IDEInstInteractionAnalysis>(EntryPoints, EdgeFactGen);
}

} // namespace psr
