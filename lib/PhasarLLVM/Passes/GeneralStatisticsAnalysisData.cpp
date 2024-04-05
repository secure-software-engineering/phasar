/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysisData.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

namespace psr {

static GeneralStatisticsAnalysisData
getDataFromJson(const nlohmann::json &Json) {
  GeneralStatisticsAnalysisData Data;

  Data.Functions = Json["Functions"];
  Data.ExternalFunctions = Json["ExternalFunctions"];
  Data.FunctionDefinitions = Json["FunctionDefinitions"];
  Data.AddressTakenFunctions = Json["AddressTakenFunctions"];
  Data.Globals = Json["Globals"];
  Data.GlobalConsts = Json["GlobalConsts"];
  Data.ExternalGlobals = Json["ExternalGlobals"];
  Data.BasicBlocks = Json["BasicBlocks"];
  Data.AllocationSites = Json["AllocationSites"];
  Data.CallSites = Json["CallSites"];
  Data.DebugIntrinsics = Json["DebugIntrinsics"];
  Data.Instructions = Json["Instructions"];
  Data.StoreInstructions = Json["StoreInstructions"];
  Data.LoadInstructions = Json["LoadInstructions"];
  Data.MemIntrinsics = Json["MemIntrinsics"];
  Data.Branches = Json["Branches"];
  Data.Switches = Json["Switches"];
  Data.GetElementPtrs = Json["GetElementPtrs"];
  Data.LandingPads = Json["LandingPads"];
  Data.PhiNodes = Json["PhiNodes"];
  Data.NumInlineAsm = Json["NumInlineAsm"];
  Data.IndCalls = Json["IndCalls"];
  Data.TotalNumOperands = Json["TotalNumOperands"];
  Data.TotalNumUses = Json["TotalNumUses"];
  Data.TotalNumPredecessorBBs = Json["TotalNumPredecessorBBs"];
  Data.TotalNumSuccessorBBs = Json["TotalNumSuccessorBBs"];
  Data.MaxNumOperands = Json["MaxNumOperands"];
  Data.MaxNumUses = Json["MaxNumUses"];
  Data.MaxNumPredecessorBBs = Json["MaxNumPredecessorBBs"];
  Data.MaxNumSuccessorBBs = Json["MaxNumSuccessorBBs"];
  Data.NumInstWithMultipleUses = Json["NumInstWithMultipleUses"];
  Data.NumInstsUsedOutsideBB = Json["NumInstsUsedOutsideBB"];
  Data.NonVoidInsts = Json["NonVoidInsts"];

  for (const auto &Curr : Json["AllocatedTypes"]) {
    Data.AllocatedTypes.insert(Curr.get<unsigned long>());
  }

  for (const auto &Curr : Json["AllocaInstructions"]) {
    Data.AllocaInstructions.insert(Curr.get<unsigned long>());
  }

  for (const auto &Curr : Json["RetResInstructions"]) {
    Data.RetResInstructions.insert(Curr.get<unsigned long>());
  }

  Data.ModuleName = Json["ModuleName"];

  return Data;
}

void GeneralStatisticsAnalysisData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json Json;

  Json["Functions"] = Functions;
  Json["ExternalFunctions"] = ExternalFunctions;
  Json["FunctionDefinitions"] = FunctionDefinitions;
  Json["AddressTakenFunctions"] = AddressTakenFunctions;
  Json["Globals"] = Globals;
  Json["GlobalConsts"] = GlobalConsts;
  Json["ExternalGlobals"] = ExternalGlobals;
  Json["BasicBlocks"] = BasicBlocks;
  Json["AllocationSites"] = AllocationSites;
  Json["CallSites"] = CallSites;
  Json["DebugIntrinsics"] = DebugIntrinsics;
  Json["Instructions"] = Instructions;
  Json["StoreInstructions"] = StoreInstructions;
  Json["LoadInstructions"] = LoadInstructions;
  Json["MemIntrinsics"] = MemIntrinsics;
  Json["Branches"] = Branches;
  Json["Switches"] = Switches;
  Json["GetElementPtrs"] = GetElementPtrs;
  Json["LandingPads"] = LandingPads;
  Json["PhiNodes"] = PhiNodes;
  Json["NumInlineAsm"] = NumInlineAsm;
  Json["IndCalls"] = IndCalls;
  Json["TotalNumOperands"] = TotalNumOperands;
  Json["TotalNumUses"] = TotalNumUses;
  Json["TotalNumPredecessorBBs"] = TotalNumPredecessorBBs;
  Json["TotalNumSuccessorBBs"] = TotalNumSuccessorBBs;
  Json["MaxNumOperands"] = MaxNumOperands;
  Json["MaxNumUses"] = MaxNumUses;
  Json["MaxNumPredecessorBBs"] = MaxNumPredecessorBBs;
  Json["MaxNumSuccessorBBs"] = MaxNumSuccessorBBs;
  Json["NumInstWithMultipleUses"] = NumInstWithMultipleUses;
  Json["NumInstsUsedOutsideBB"] = NumInstsUsedOutsideBB;
  Json["NonVoidInsts"] = NonVoidInsts;

  for (const auto &Curr : AllocatedTypes) {
    Json["AllocatedTypes"].push_back(Curr);
  }

  for (const auto &Curr : AllocaInstructions) {
    Json["AllocaInstructions"].push_back(Curr);
  }

  for (const auto &Curr : RetResInstructions) {
    Json["RetResInstructions"].push_back(Curr);
  }

  Json["ModuleName"] = ModuleName;

  OS << Json;
}

GeneralStatisticsAnalysisData
GeneralStatisticsAnalysisData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

GeneralStatisticsAnalysisData
GeneralStatisticsAnalysisData::loadJsonString(llvm::StringRef JsonAsString) {
  nlohmann::json Data =
      nlohmann::json::parse(JsonAsString.begin(), JsonAsString.end());
  return getDataFromJson(Data);
}

} // namespace psr
