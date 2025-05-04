/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_VARSTATICRENAMING_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_VARSTATICRENAMING_H_

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include <map>
#include <string>

namespace psr {

using stringstringmap_t = // std::map<std::string, T>;
    llvm::StringMap<llvm::StringRef>;

/// Extracts the mapping unmangledName -> ctx_mangledName from the given
/// ProjectIRDB
stringstringmap_t extractStaticRenaming(const LLVMProjectIRDB *IRDB);

/// Extracts the mappings (unmangledName -> ctx_mangledName and ctx_mangledName
/// -> unmangledName) from the given ProjectIRDB
std::pair<stringstringmap_t, stringstringmap_t>
extractBiDiStaticRenaming(const LLVMProjectIRDB *IRDB);

std::string
getDemangledFunctionName(llvm::StringRef Name,
                         const stringstringmap_t *StaticBackwardRenaming);

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_VARSTATICRENAMING_H_