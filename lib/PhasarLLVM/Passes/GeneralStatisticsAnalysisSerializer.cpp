/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysisSerializer.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

namespace psr {

void GeneralStatisticsAnalysisSerializer::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json Json;

  Json["Functions"] = Functions;
  Json["ExternalFunctions"] = ExternalFunctions;
  Json["FunctionDefinitions"] = FunctionDefinitions;
  Json["AddressTakenFunctions"] = AddressTakenFunctions;
  Json["Globals"] = Globals;
  Json["GlobalConsts"] = GlobalConsts;
  Json["BasicBlocks"] = BasicBlocks;
  Json["CallSites"] = CallSites;
  Json["DebugIntrinsics"] = DebugIntrinsics;
  Json["Instructions"] = Instructions;
  Json["MemIntrinsics"] = MemIntrinsics;
  Json["Branches"] = Branches;
  Json["GetElementPtrs"] = GetElementPtrs;
  Json["LandingPads"] = LandingPads;
  Json["PhiNodes"] = PhiNodes;
  Json["NumInlineAsm"] = NumInlineAsm;
  Json["IndCalls"] = IndCalls;
  Json["NonVoidInsts"] = NonVoidInsts;
  Json["ModuleName"] = ModuleName;

  OS << Json;
}

} // namespace psr
