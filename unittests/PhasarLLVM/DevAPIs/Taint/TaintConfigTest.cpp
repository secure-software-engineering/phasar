/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert
 *****************************************************************************/

#include <string>

#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DevAPIs/Taint/TaintConfig.h"
#include "phasar/Utils/LLVMShorthands.h"

#include "TestConfig.h"

using namespace psr;
using namespace psr::unittest;

TEST(TaintConfigTest, TestDeclareVarAsSource) {
  const std::string PathToLlFiles =
      unittest::PathToLLTestFiles + "full_constant/";
  ProjectIRDB IR({PathToLlFiles + ""}, IRDBOptions::WPA);
  // TODO test declare var as source function
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
