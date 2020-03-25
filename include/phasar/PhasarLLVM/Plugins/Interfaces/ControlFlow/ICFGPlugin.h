/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_CONTROLFLOW_ICFGPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_CONTROLFLOW_ICFGPLUGIN_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"

namespace llvm {
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class ProjectIRDB;

class ICFGPlugin
    : public ICFG<const llvm::Instruction *, const llvm::Function *> {
private:
  [[maybe_unused]] ProjectIRDB &IRDB;
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
