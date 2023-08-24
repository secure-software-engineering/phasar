/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar.h"

#include <filesystem>
#include <string>

using namespace psr;

int main(int Argc, const char **Argv) {
  std::string File =
      "/home/max/Desktop/Arbeit/phasar-f-TaintConfigSer/phasar/build/test/"
      "llvm_test_code/TaintConfig/JsonConfig/array_01_c_dbg.ll";
  std::string Config =
      "/home/max/Desktop/Arbeit/phasar-f-TaintConfigSer/phasar/build/test/"
      "llvm_test_code/TaintConfig/JsonConfig/array_01_config.json";
  llvm::outs() << "Test 0\n";
  llvm::outs().flush();
  llvm::outs() << Config << "\n";
  llvm::outs() << File << "\n";
  llvm::outs().flush();
  auto JsonConfig = psr::TaintConfigData({Config});
  llvm::outs() << "Test 1\n";
  llvm::outs().flush();

  psr::LLVMProjectIRDB IR({File});
  llvm::outs() << "Test 2\n";
  llvm::outs().flush();
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  llvm::outs() << "Test 3\n";
  llvm::outs().flush();
  llvm::outs() << TConfig << '\n';
  const llvm::Value *I = IR.getInstruction(3);
  // ASSERT_TRUE(TConfig.isSource(I));
  llvm::outs() << "Test 4\n";
  llvm::outs().flush();
}
