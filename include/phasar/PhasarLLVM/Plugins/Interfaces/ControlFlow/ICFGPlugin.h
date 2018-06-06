/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef ICFGPlugin_H_
#define ICFGPlugin_H_

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <map>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <string>

namespace psr {

class ICFGPlugin
    : public ICFG<const llvm::Instruction *, const llvm::Function *> {
private:
  ProjectIRDB &IRDB;
  const std::vector<std::string> EntryPoints;

public:
  ICFGPlugin(ProjectIRDB &IRDB, const std::vector<std::string> EntryPoints)
      : IRDB(IRDB), EntryPoints(move(EntryPoints)) {}
};

extern std::map<std::string,
                std::unique_ptr<ICFGPlugin> (*)(
                    ProjectIRDB &, const std::vector<std::string> EntryPoints)>
    ICFGPluginFactory;

} // namespace psr

#endif