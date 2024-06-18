/******************************************************************************
 * Copyright (c) 2024 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Passes/OpaquePtrTyPass.h"

#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"

#include <map>

namespace psr {

OpaquePtrTyPass::OpaquePtrTyPass() : llvm::ModulePass(ID) {}

llvm::StringRef OpaquePtrTyPass::getPassName() const {
  return "OpaquePtrTyPass";
}

bool OpaquePtrTyPass::runOnModule(llvm::Module &M) {
  llvm::outs() << "OpaquePtrTyPass::runOnModule()\n";

  std::map<std::string, std::string> PtrTypes;

  // get pointer types
  for (const auto &Func : M.getFunctionList()) {
    if (Func.isDeclaration()) {
      continue;
    }

    PtrTypes[Func.getName().str()] =
        Func.getFunctionType()->getStructName().str();
  }

  // save pointer types to json file
  nlohmann::json Json(PtrTypes);
  std::string PathToJson = "./OpaquePtrTyPassJsons/";
  std::string FileName = PathToJson + "PointerTypes.json";

  llvm::sys::fs::create_directories(PathToJson);
  std::error_code EC;
  llvm::raw_fd_ostream FileStream(llvm::StringRef(FileName), EC);

  if (EC) {
    PHASAR_LOG_LEVEL(ERROR, EC.message());
    return false;
  }

  FileStream << Json;

  llvm::outs() << "Json with pointer types saved to: " << PathToJson << "\n";

  return true;
}
static llvm::RegisterPass<OpaquePtrTyPass>
    Phasar("opaque-pointer-type-pass", "PhASAR Opaque Pointer Type Pass", false,
           false);

} // namespace psr
