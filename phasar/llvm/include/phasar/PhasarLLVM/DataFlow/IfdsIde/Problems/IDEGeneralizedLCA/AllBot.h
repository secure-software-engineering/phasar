/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_ALLBOT_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_ALLBOT_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr::glca {

// struct AllBot {
//   using type = AllBottom<IDEGeneralizedLCA::l_t>;
//   static std::shared_ptr<type> getInstance();
//   static bool isBot(const EdgeFunction<IDEGeneralizedLCA::l_t> *EdgeFn,
//                     bool NonRec = false);
//   static bool
//   isBot(const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &EdgeFn,
//         bool NonRec = false);
// };

} // namespace psr::glca

#endif
