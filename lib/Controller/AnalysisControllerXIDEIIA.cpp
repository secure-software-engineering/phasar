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
  // use Phasar's instruction ids as testing labels
  auto Generator =
      [](std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>
             Current) -> std::set<std::string> {
    return std::visit(
        [](const auto *InstOrGlob) -> std::set<std::string> {
          std::set<std::string> Labels;
          if (InstOrGlob->hasMetadata()) {
            std::string Label =
                llvm::cast<llvm::MDString>(
                    InstOrGlob->getMetadata(PhasarConfig::MetaDataKind())
                        ->getOperand(0))
                    ->getString()
                    .str();
            Labels.insert(Label);
          }
          return Labels;
        },
        Current);
  };

  executeIDEAnalysis<IDEInstInteractionAnalysis>(EntryPoints, Generator);
}

} // namespace psr
