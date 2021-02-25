/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_PLUGINCTORS_H_
#define PHASAR_PHASARLLVM_PLUGINS_PLUGINCTORS_H_

#include <memory>
#include <set>
#include <string>

namespace psr {

class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedCFG;
class LLVMBasedICFG;
class LLVMPointsToInfo;
class IFDSTabulationProblemPlugin;
class IDETabulationProblemPlugin;
class IntraMonoProblemPlugin;
class InterMonoProblemPlugin;

using IFDSPluginConstructor = std::unique_ptr<IFDSTabulationProblemPlugin> (*)(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints);

using IDEPluginConstructor = std::unique_ptr<IDETabulationProblemPlugin> (*)(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints);

using IntraMonoPluginConstructor = std::unique_ptr<IntraMonoProblemPlugin> (*)(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedCFG *CF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints);

using InterMonoPluginConstructor = std::unique_ptr<InterMonoProblemPlugin> (*)(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints);

} // namespace psr

#endif
